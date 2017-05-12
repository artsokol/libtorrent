#include "libtorrent/navigable_small_world/traversal_algorithm.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/socket_io.hpp" // for read_*_endpoint
#include "libtorrent/alert_types.hpp" // for nsw_lookup
#include "libtorrent/aux_/time.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif
using namespace std::placeholders;

namespace libtorrent { namespace nsw
{

#if TORRENT_USE_ASSERTS
template <class It, class Cmp>
bool is_sorted(It b, It e, Cmp cmp)
{
	return true;
}
#endif

observer_ptr traversal_algorithm::new_observer(udp::endpoint const& ep
	, node_id const& id
	, vector_t const& descr)
{
	auto o = m_node.m_rpc.allocate_observer<null_observer>(self(), ep, id, descr);
	return o;
}

traversal_algorithm::traversal_algorithm(
	node& nsw_node
	, node_id const& id
	, vector_t const& target_string)
	: m_node(nsw_node)
	, m_nid(nsw_node.nid())
	, m_target(target_string)
{
#ifndef TORRENT_DISABLE_LOGGING
	m_id = m_node.search_id();
	nsw_logger_observer_interface* logger = get_node().observer();
	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
	{ // provoke corruption. check %u
		// logger->nsw_log(nsw_logger_interface::traversal, "[%u] NEW target: %s k: %d"
		// 	, m_id, term_vector::vectorToString(target_string).c_str(), m_node.m_table.neighbourhood_size());
	}
#endif
}

void traversal_algorithm::resort_results()
{
	std::sort(m_results.begin(), m_results.end()
		, [this](observer_ptr const& lhs, observer_ptr const& rhs)
		{ return term_vector::getVecSimilarity(lhs->descr(),m_node.m_table.get_descr()) >
					term_vector::getVecSimilarity(rhs->descr(),m_node.m_table.get_descr()); });
}

void traversal_algorithm::add_entry(node_id const& id
								, vector_t const& description
								, udp::endpoint const& addr
								, unsigned char const flags)
{
//	TORRENT_ASSERT(m_node.m_rpc.allocation_size() >= sizeof(find_data_observer));
	auto o = new_observer(addr, id, description);
	if (!o)
	{
#ifndef TORRENT_DISABLE_LOGGING
		if (get_node().observer() != nullptr)
		{
			get_node().observer()->nsw_log(nsw_logger_interface::traversal, "[%u] failed to allocate memory or observer. aborting!"
				, m_id);
		}
#endif
		done();
		return;
	}
	// if (id.is_all_zeros())
	// {
	// 	o->set_id(generate_random_id());
	// 	o->flags |= observer_interface::flag_no_id;
	// }

	o->flags |= flags;

	// TORRENT_ASSERT(libtorrent::nsw::is_sorted(m_results.begin(), m_results.end()
	// 	, [this](observer_ptr const& lhs, observer_ptr const& rhs)
	// 	{ return compare_ref(lhs->id(), rhs->id(), m_target); }));

	// auto iter = std::lower_bound(m_results.begin(), m_results.end(), o
	// 	, [this](observer_ptr const& lhs, observer_ptr const& rhs)
	// 	{ return compare_ref(lhs->id(), rhs->id(), m_target); });
	auto iter = std::find_if(m_results.begin(), m_results.end()
				, [this,&description](observer_ptr const& lhs)
				{ return term_vector::getVecSimilarity(lhs->descr(),m_node.m_table.get_descr()) <
					term_vector::getVecSimilarity(description,m_node.m_table.get_descr()); });

	if (iter == m_results.end() || (*iter)->id() != id) /// check text description?
	{
// 		if (m_node.settings().restrict_search_ips
// 			&& !(flags & observer_interface::flag_initial))
// 		{
// #if TORRENT_USE_IPV6
// 			if (o->target_addr().is_v6())
// 			{
// 				address_v6::bytes_type addr_bytes = o->target_addr().to_v6().to_bytes();
// 				address_v6::bytes_type::const_iterator prefix_it = addr_bytes.begin();
// 				std::uint64_t const prefix6 = detail::read_uint64(prefix_it);

// 				if (m_peer6_prefixes.insert(prefix6).second)
// 					goto add_result;
// 			}
// 			else
// #endif
// 			{
// 				// mask the lower octet
// 				std::uint32_t const prefix4
// 					= o->target_addr().to_v4().to_ulong() & 0xffffff00;

// 				if (m_peer4_prefixes.insert(prefix4).second)
// 					goto add_result;
// 			}

// 			// we already have a node in this search with an IP very
// 			// close to this one. We know that it's not the same, because
// 			// it claims a different node-ID. Ignore this to avoid attacks
// #ifndef TORRENT_DISABLE_LOGGING
// 			nsw_logger_observer_interface* logger = get_node().observer();
// 			if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
// 			{
// 				logger->nsw_log(nsw_logger_interface::traversal
// 					, "[%u] traversal DUPLICATE node. id: %s addr: %s type: %s"
// 					, m_id, aux::to_hex(o->id()).c_str(), print_address(o->target_addr()).c_str(), name());
// 			}
// #endif
// 			return;
// 		}

// 	add_result:

		// TORRENT_ASSERT((o->flags & observer_interface::flag_no_id)
		// 	|| std::none_of(m_results.begin(), m_results.end()
		// 	, [&id](observer_ptr const& ob) { return ob->id() == id; }));

#ifndef TORRENT_DISABLE_LOGGING
		nsw_logger_observer_interface* logger = get_node().observer();
		if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
		{
			logger->nsw_log(nsw_logger_interface::traversal
				, "[%u] ADD id: %s addr: %s invoke-count: %d type: %s"
				, m_id, aux::to_hex(id).c_str(), print_endpoint(addr).c_str()
				, m_invoke_count, name());
		}
#endif
		// --iter;
		// iter = m_results.insert(iter, o);
		m_results.push_back(o);
		// TORRENT_ASSERT(libtorrent::dht::is_sorted(m_results.begin(), m_results.end()
		// 	, [this](observer_ptr const& lhs, observer_ptr const& rhs)
		// 	{ return compare_ref(lhs->id(), rhs->id(), m_target); }));
	}

	if (m_results.size() > 100)
	{
		std::for_each(m_results.begin() + 100, m_results.end()
			, [this](std::shared_ptr<observer_interface> const& ptr)
		{
			if ((ptr->flags & (observer_interface::flag_queried | observer_interface::flag_failed | observer_interface::flag_alive))
				== observer_interface::flag_queried)
			{
				// set the done flag on any outstanding queries to prevent them from
				// calling finished() or failed()
				ptr->flags |= observer_interface::flag_done;
				TORRENT_ASSERT(m_invoke_count > 0);
				--m_invoke_count;
			}

		});
		m_results.resize(100);
	}
}

void traversal_algorithm::start()
{
	// in case the routing table is empty, use the
	// router nodes in the table
	if (m_results.size() < 3) add_gate_entries();
	init();
	bool is_done = add_requests();
	if (is_done) done();
}

char const* traversal_algorithm::name() const
{
	return "traversal_algorithm";
}

// void traversal_algorithm::traverse(node_id const& id, udp::endpoint const& addr)
// {
// #ifndef TORRENT_DISABLE_LOGGING
// 	nsw_nsw_logger_observer_interface* logger = get_node().observer();
// 	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal) && id.is_all_zeros())
// 	{
// 		logger->nsw_log(nsw_logger_interface::traversal
// 			, "[%u] WARNING node returned a list which included a node with id 0"
// 			, m_id);
// 	}
// #endif

// 	// let the routing table know this node may exist
// 	m_node.m_table.heard_about(id, addr);

// 	add_entry(id, addr, 0);
// }

void traversal_algorithm::finished(observer_ptr o)
{
	// if this flag is set, it means we increased the
	// branch factor for it, and we should restore it
	if (o->flags & observer_interface::flag_short_timeout)
	{
		--m_max_threads;
	}

	o->flags |= observer_interface::flag_alive;

	++m_responses;
	--m_invoke_count;
	bool is_done = add_requests();
	if (is_done) done();
}

// prevent request means that the total number of requests has
// overflown. This query failed because it was the oldest one.
// So, if this is true, don't make another request
void traversal_algorithm::failed(observer_ptr o, int const flags)
{
	// don't tell the routing table about
	// node ids that we just generated ourself
	if ((o->flags & observer_interface::flag_no_id) == 0)
		m_node.m_table.node_failed(o->id(), o->target_ep());

	if (m_results.empty()) return;

	bool decrement_branch_factor = false;

//	TORRENT_ASSERT(o->flags & observer_interface::flag_queried);
	if (flags & short_timeout)
	{
		// short timeout means that it has been more than
		// two seconds since we sent the request, and that
		// we'll most likely not get a response. But, in case
		// we do get a late response, keep the handler
		// around for some more, but open up the slot
		// by increasing the branch factor
		if ((o->flags & observer_interface::flag_short_timeout) == 0)
		{
//			TORRENT_ASSERT(m_max_threads < (std::numeric_limits<std::int16_t>::max)());
			++m_max_threads;
		}
		o->flags |= observer_interface::flag_short_timeout;
#ifndef TORRENT_DISABLE_LOGGING
		log_timeout(o, "1ST_");
#endif
	}
	else
	{
		o->flags |= observer_interface::flag_failed;
		// if this flag is set, it means we increased the
		// branch factor for it, and we should restore it
		decrement_branch_factor = (o->flags & observer_interface::flag_short_timeout) != 0;

#ifndef TORRENT_DISABLE_LOGGING
		log_timeout(o,"");
#endif

		++m_timeouts;
//		TORRENT_ASSERT(m_invoke_count > 0);
		--m_invoke_count;
	}

	// this is another reason to decrement the branch factor, to prevent another
	// request from filling this slot. Only ever decrement once per response though
	decrement_branch_factor |= (flags & prevent_request);

	if (decrement_branch_factor)
	{
//		TORRENT_ASSERT(m_max_threads > 0);
		--m_max_threads;
		if (m_max_threads <= 0) m_max_threads = 1;
	}

	bool const is_done = add_requests();
	if (is_done) done();
}

#ifndef TORRENT_DISABLE_LOGGING
void traversal_algorithm::log_timeout(observer_ptr const& o, char const* prefix) const
{
	nsw_logger_observer_interface * logger = get_node().observer();
	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
	{
		logger->nsw_log(nsw_logger_interface::traversal
			, "[%u] %sTIMEOUT id: %s addr: %s branch-factor: %d "
			"invoke-count: %d type: %s"
			, m_id, prefix, aux::to_hex(o->id()).c_str()
			, print_address(o->target_addr()).c_str(), m_max_threads
			, m_invoke_count, name());
	}
}
#endif

void traversal_algorithm::done()
{
#ifndef TORRENT_DISABLE_LOGGING
	int results_target = m_node.m_table.neighbourhood_size();
//	int closest_target = 160;
#endif

	for (auto const& o : m_results)
	{
		if ((o->flags & (observer_interface::flag_queried | observer_interface::flag_failed)) == observer_interface::flag_queried)
		{
			// set the done flag on any outstanding queries to prevent them from
			// calling finished() or failed() after we've already declared the traversal
			// done
			o->flags |= observer_interface::flag_done;
		}
	}

#ifndef TORRENT_DISABLE_LOGGING
	if (get_node().observer() != nullptr)
	{
		get_node().observer()->nsw_log(nsw_logger_interface::traversal
			, "[%u] COMPLETED type: %s"
			, m_id, name());
	}
#endif

	// delete all our references to the observer objects so
	// they will in turn release the traversal algorithm
	m_results.clear();
	m_invoke_count = 0;
}

bool traversal_algorithm::add_requests()
{
	int results_target = m_node.m_table.neighbourhood_size();

	// this only counts outstanding requests at the top of the
	// target list. This is <= m_invoke count. m_invoke_count
	// is the total number of outstanding requests, including
	// old ones that may be waiting on nodes much farther behind
	// the current point we've reached in the search.
	int outstanding = 0;

	// if we're doing aggressive lookups, we keep branch-factor
	// outstanding requests _at the tops_ of the result list. Otherwise
	// we just keep any branch-factor outstanding requests
	// bool agg = m_node.settings().aggressive_lookups;

	// Find the first node that hasn't already been queried.
	// and make sure that the 'm_max_threads' top nodes
	// stay queried at all times (obviously ignoring failed nodes)
	// and without surpassing the 'result_target' nodes (i.e. k=8)
	// this is a slight variation of the original paper which instead
	// limits the number of outstanding requests, this limits the
	// number of good outstanding requests. It will use more traffic,
	// but is intended to speed up lookups
	for (std::vector<observer_ptr>::iterator i = m_results.begin()
		, end(m_results.end()); i != end
		&& results_target > 0
		&& (m_invoke_count < m_max_threads);
		++i)
	{
		observer_interface* o = i->get();
		if (o->flags & observer_interface::flag_alive)
		{
			--results_target;
			continue;
		}
		if (o->flags & observer_interface::flag_queried)
		{
			// if it's queried, not alive and not failed, it
			// must be currently in flight
			if ((o->flags & observer_interface::flag_failed) == 0)
				++outstanding;

			continue;
		}

#ifndef TORRENT_DISABLE_LOGGING
		nsw_logger_observer_interface* logger = get_node().observer();
		if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
		{
			logger->nsw_log(nsw_logger_interface::traversal
				, "[%u] INVOKE nodes-left: %d top-invoke-count: %d "
				"invoke-count: %d branch-factor: %d "
				"id: %s addr: %s type: %s"
				, m_id, int(m_results.end() - i), outstanding, int(m_invoke_count)
				, int(m_max_threads), aux::to_hex(o->id()).c_str()
				, print_address(o->target_addr()).c_str(), name());
		}
#endif

		o->flags |= observer_interface::flag_queried;
		if (invoke(*i))
		{
//			TORRENT_ASSERT(m_invoke_count < (std::numeric_limits<std::int16_t>::max)());
			++m_invoke_count;
			++outstanding;
		}
		else
		{
			o->flags |= observer_interface::flag_failed;
		}
	}

	// this is the completion condition. If we found m_node.m_table.bucket_size()
	// (i.e. k=8) completed results, without finding any still
	// outstanding requests, we're done.
	// also, if invoke count is 0, it means we didn't even find 'k'
	// working nodes, we still have to terminate though.
	return (results_target == 0 && outstanding == 0) || m_invoke_count == 0;
}

void traversal_algorithm::add_gate_entries()
{
#ifndef TORRENT_DISABLE_LOGGING
	nsw_logger_observer_interface* logger = get_node().observer();
	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
	{
		logger->nsw_log(nsw_logger_interface::traversal
			, "[%u] using router nodes to initiate traversal algorithm %d routers"
			, m_id, int(std::distance(m_node.m_table.begin(), m_node.m_table.end())));
	}
#endif
	for (auto const& n : m_node.m_table)
		add_entry(node_id(0), n.first, n.second, observer_interface::flag_initial);
}

void traversal_algorithm::init()
{
	m_max_threads = std::int16_t(m_node.get_search_threads());
	m_node.add_traversal_algorithm(this);
}

traversal_algorithm::~traversal_algorithm()
{
	m_node.remove_traversal_algorithm(this);
}

void traversal_algorithm::status(nsw_lookup& l)
{
	l.timeouts = m_timeouts;
	l.responses = m_responses;
	l.in_progress_requests = m_invoke_count;
	l.threads = m_max_threads;
	l.type = name();
	//l.nodes_left = 0;
	//l.first_timeout = 0;
	//l.target = m_target;
}

void traversal_observer::reply(msg const& m)
{
	bdecode_node r = m.message.dict_find_dict("r");
	if (!r)
	{
#ifndef TORRENT_DISABLE_LOGGING
		if (get_observer() != nullptr)
		{
			get_observer()->nsw_log(nsw_logger_interface::traversal
				, "[%u] missing response dict"
				, algorithm()->id());
		}
#endif
		return;
	}

#ifndef TORRENT_DISABLE_LOGGING
	nsw_logger_observer_interface* logger = get_observer();
	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
	{
		bdecode_node nid = r.dict_find_string("r_id");
		char hex_id[41];
		aux::to_hex({nid.string_ptr(), 20}, hex_id);
		logger->nsw_log(nsw_logger_interface::traversal
			, "[%u] RESPONSE id: %s invoke-count: %d addr: %s type: %s"
			, algorithm()->id(), hex_id, algorithm()->invoke_count()
			, print_endpoint(target_ep()).c_str(), algorithm()->name());
	}
#endif

	// look for nodes
	// udp const protocol = algorithm()->get_node().get_protocol();
	// int const protocol_size = int(detail::address_size(protocol));
	// char const* nodes_key = "friends"; //algorithm()->get_node().protocol_nodes_key();
	// bdecode_node n = r.dict_find_string(nodes_key);
	// if (n)
	// {
	// 	// handle friends list
	// 	// char const* nodes = n.string_ptr();
	// 	// char const* end = nodes + n.string_length();

	// 	// while (end - nodes >= 20 + protocol_size + 2)
	// 	// {
	// 	// 	node_endpoint nep = read_node_endpoint(protocol, nodes);
	// 	// 	algorithm()->traverse(nep.id, nep.ep);
	// 	// }
	// }

	bdecode_node id = r.dict_find_string("r_id");
	if (!id || id.string_length() != 20)
	{
#ifndef TORRENT_DISABLE_LOGGING
		if (get_observer() != nullptr)
		{
			get_observer()->nsw_log(nsw_logger_interface::traversal, "[%u] invalid id in response"
				, algorithm()->id());
		}
#endif
		return;
	}

	// in case we didn't know the id of this peer when we sent the message to
	// it. For instance if it's a bootstrap node.
	//set_id(node_id(id.string_ptr()));
}

} } // namespace libtorrent::nsw
