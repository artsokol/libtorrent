#include "libtorrent/config.hpp"
#include "libtorrent/io.hpp"
#include "libtorrent/random.hpp"
#include "libtorrent/invariant_check.hpp"

#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/traversal_algorithm.hpp"
#include "libtorrent/socket_io.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/session_settings.hpp"
#include "libtorrent/aux_/time.hpp"

#include <type_traits>
#include <functional>

#ifndef TORRENT_DISABLE_LOGGING
#include <cinttypes>
#endif

using namespace std::placeholders;

namespace libtorrent { namespace nsw {

using observer_storage = std::aligned_union<1
//	, find_data_observer
//	, get_item_observer
//	, get_peers_observer
//  , traversal_observer
	,null_observer>::type;

rpc_manager::rpc_manager(node_id const& our_id
	, nsw_settings const& settings
	, routing_table& table, udp_socket_interface* sock
	, nsw_logger_interface* log)
	: m_pool_allocator(sizeof(observer_storage), 10)
	, m_sock(sock)
#ifndef TORRENT_DISABLE_LOGGING
	, m_log(log)
#endif
	, m_settings(settings)
	, m_table(table)
	, m_our_id(our_id)
	, m_allocated_observers(0)
	, m_destructing(false)
{
#ifdef TORRENT_DISABLE_LOGGING
	TORRENT_UNUSED(log);
#endif
}

rpc_manager::~rpc_manager()
{
	TORRENT_ASSERT(!m_destructing);
	m_destructing = true;

	for (auto const& t : m_transactions)
	{
		t.second->abort();
	}
}

inline void* rpc_manager::allocate_observer()
{
	m_pool_allocator.set_next_size(10);
	void* ret = m_pool_allocator.malloc();
	if (ret != nullptr) ++m_allocated_observers;
	return ret;
}

inline void rpc_manager::free_observer(void* ptr)
{
	if (ptr == nullptr) return;
	--m_allocated_observers;
	TORRENT_ASSERT(reinterpret_cast<observer_interface*>(ptr)->m_in_use == false);
	m_pool_allocator.free(ptr);
}


void rpc_manager::unreachable(udp::endpoint const& ep)
{
#ifndef TORRENT_DISABLE_LOGGING
	if (m_log->should_log(nsw_logger_interface::rpc_manager))
	{
		m_log->log(nsw_logger_interface::rpc_manager, "MACHINE_UNREACHABLE [ ip: %s ]"
			, print_endpoint(ep).c_str());
	}
#endif

	for (auto i = m_transactions.begin(); i != m_transactions.end();)
	{
		TORRENT_ASSERT(i->second);
		observer_ptr const& o = i->second;
		if (o->target_ep() != ep) { ++i; continue; }
		observer_ptr ptr = i->second;
		i = m_transactions.erase(i);
#ifndef TORRENT_DISABLE_LOGGING
		m_log->log(nsw_logger_interface::rpc_manager, "found transaction [ tid: %d ]"
			, int(ptr->transaction_id()));
#endif
		ptr->timeout();
		break;
	}
}

bool rpc_manager::incoming(msg const& m, node_id* id)
{
	if (m_destructing) return false;

	// we only deal with replies and errors, not queries
	TORRENT_ASSERT(m.message.dict_find_string_value("y") == "r"
		|| m.message.dict_find_string_value("y") == "e");

	// if we don't have the transaction id in our
	// request list, ignore the packet

	auto transaction_id = m.message.dict_find_string_value("t");
	if (transaction_id.empty()) return false;

	auto ptr = transaction_id.begin();
	int tid = transaction_id.size() != 2 ? -1 : detail::read_uint16(ptr);

	observer_ptr o;
	auto range = m_transactions.equal_range(tid);
	for (auto i = range.first; i != range.second; ++i)
	{
		if (m.addr.address() != i->second->target_addr()) continue;
		o = i->second;
		i = m_transactions.erase(i);
		break;
	}

	if (!o)
	{
#ifndef TORRENT_DISABLE_LOGGING
		if (m_table.is_native_endpoint(m.addr) && m_log->should_log(nsw_logger_interface::rpc_manager))
		{
			m_log->log(nsw_logger_interface::rpc_manager, "reply with unknown transaction id size: %d from %s"
				, int(transaction_id.size()), print_endpoint(m.addr).c_str());
		}
#endif
		// this isn't necessarily because the other end is doing
		// something wrong. This can also happen when we restart
		// the node, and we prematurely abort all outstanding
		// requests. Also, this opens up a potential magnification
		// attack.
		return false;
	}

	time_point const now = clock_type::now();

#ifndef TORRENT_DISABLE_LOGGING
	if (m_log->should_log(nsw_logger_interface::rpc_manager))
	{
		m_log->log(nsw_logger_interface::rpc_manager, "round trip time(ms): %" PRId64 " from %s"
			, total_milliseconds(now - o->sent()), print_endpoint(m.addr).c_str());
	}
#endif

	if (m.message.dict_find_string_value("y") == "e")
	{
		// It's an error.
#ifndef TORRENT_DISABLE_LOGGING
		if (m_log->should_log(nsw_logger_interface::rpc_manager))
		{
			bdecode_node err = m.message.dict_find_list("e");
			if (err && err.list_size() >= 2
				&& err.list_at(0).type() == bdecode_node::int_t
				&& err.list_at(1).type() == bdecode_node::string_t)
			{
				m_log->log(nsw_logger_interface::rpc_manager, "reply with error from %s: (%" PRId64 ") %s"
					, print_endpoint(m.addr).c_str()
					, err.list_int_value_at(0)
					, err.list_string_value_at(1).to_string().c_str());
			}
			else
			{
				m_log->log(nsw_logger_interface::rpc_manager, "reply with (malformed) error from %s"
					, print_endpoint(m.addr).c_str());
			}
		}
#endif
		// Logically, we should call o->reply(m) since we get a reply.
		// a reply could be "response" or "error", here the reply is an "error".
		// if the reply is an "error", basically the observer could/will
		// do nothing with it, especially when observer::reply() is intended to
		// handle a "response", not an "error".
		// A "response" should somehow call algorithm->finished(), and an error/timeout
		// should call algorithm->failed(). From this point of view,
		// we should call o->timeout() instead of o->reply(m) because o->reply()
		// will call algorithm->finished().
		o->timeout();
		return false;
	}

	bdecode_node ret_ent = m.message.dict_find_dict("r");
	if (!ret_ent)
	{
		o->timeout();
		return false;
	}

	bdecode_node node_id_ent = ret_ent.dict_find_string("id");
	if (!node_id_ent || node_id_ent.string_length() != 20)
	{
		o->timeout();
		return false;
	}

	node_id nid = node_id(node_id_ent.string_ptr());
	// We need no this chekickng for NSW right now, but just commenting out
	// to double check it later.
	/*if (m_settings.enforce_node_id && !verify_id(nid, m.addr.address())) // ?????
	{
		o->timeout();
		return false;
	}*/

#ifndef TORRENT_DISABLE_LOGGING
	if (m_log->should_log(nsw_logger_interface::rpc_manager))
	{
		m_log->log(nsw_logger_interface::rpc_manager, "[%p] reply with transaction id: %d from %s"
			, static_cast<void*>(o->algorithm()), int(transaction_id.size())
			, print_endpoint(m.addr).c_str());
	}
#endif
	o->reply(m);
	*id = nid;

	int rtt = int(total_milliseconds(now - o->sent()));

	// we found an observer for this reply, hence the node is not spoofing
	// add it to the routing table
	return m_table.node_seen(*id, m.addr, rtt);
}

time_duration rpc_manager::tick()
{
	constexpr int short_timeout = 1;
	constexpr int timeout = 15;

	// look for observers that have timed out

	if (m_transactions.empty()) return seconds(short_timeout);

	std::vector<observer_ptr> timeouts;
	std::vector<observer_ptr> short_timeouts;

	time_duration ret = seconds(short_timeout);
	time_point now = aux::time_now();

	for (auto i = m_transactions.begin(); i != m_transactions.end();)
	{
		observer_ptr o = i->second;

		time_duration diff = now - o->sent();
		if (diff >= seconds(timeout))
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (m_log->should_log(nsw_logger_interface::rpc_manager))
			{
				m_log->log(nsw_logger_interface::rpc_manager, "[%p] timing out transaction id: %d from: %s"
					, static_cast<void*>(o->algorithm()), o->transaction_id()
					, print_endpoint(o->target_ep()).c_str());
			}
#endif
			m_transactions.erase(i++);
			timeouts.push_back(o);
			continue;
		}

		// don't call short_timeout() again if we've
		// already called it once
		if (diff >= seconds(short_timeout) && !o->has_short_timeout())
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (m_log->should_log(nsw_logger_interface::rpc_manager))
			{
				m_log->log(nsw_logger_interface::rpc_manager, "[%p] short-timing out transaction id: %d from: %s"
					, static_cast<void*>(o->algorithm()), o->transaction_id()
					, print_endpoint(o->target_ep()).c_str());
			}
#endif
			++i;

			short_timeouts.push_back(o);
			continue;
		}

		ret = std::min(seconds(timeout) - diff, ret);
		++i;
	}

	std::for_each(timeouts.begin(), timeouts.end(), std::bind(&observer_interface::timeout, _1));
	std::for_each(short_timeouts.begin(), short_timeouts.end(), std::bind(&observer_interface::short_timeout, _1));

	return (std::max)(ret, duration_cast<time_duration>(milliseconds(200)));
}

inline void rpc_manager::add_our_id(entry& e)
{
	e["id"] = m_our_id.to_string();
}

bool rpc_manager::invoke(entry& e, udp::endpoint target_addr
	, observer_ptr o)
{

	if (m_destructing) return false;

	e["y"] = "q";
	entry& a = e["a"];
	add_our_id(a);

	std::string transaction_id;
	transaction_id.resize(2);
	char* out = &transaction_id[0];
	std::uint16_t const tid = std::uint16_t(random(~0));
	detail::write_uint16(tid, out);
	e["t"] = transaction_id;


	node& n = o->algorithm()->get_node();
	if (!n.native_address(o->target_addr()))
	{
		a["want"].list().push_back(entry(n.protocol_family_name()));
	}

	o->set_target(target_addr);
	o->set_transaction_id(tid);

#ifndef TORRENT_DISABLE_LOGGING
	if (m_log != nullptr && m_log->should_log(nsw_logger_interface::rpc_manager))
	{
		m_log->log(nsw_logger_interface::rpc_manager, "[%p] invoking %s -> %s"
			, static_cast<void*>(o->algorithm()), e["q"].string().c_str()
			, print_endpoint(target_addr).c_str());
	}
#endif

	if (m_sock->send_packet(e, target_addr))
	{
		m_transactions.insert(std::make_pair(tid, o));
#if TORRENT_USE_ASSERTS
		o->m_was_sent = true;
#endif
		return true;
	}
	return false;
}

#if TORRENT_USE_ASSERTS
size_t rpc_manager::allocation_size() const
{
	return sizeof(observer_storage);
}
#endif

} }

