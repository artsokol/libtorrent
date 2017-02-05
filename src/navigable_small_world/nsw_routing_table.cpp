#include <vector>
#include <iterator>
#include <algorithm>
#include <functional>
#include <numeric>
#include <cstdio>
#include <cinttypes>
#include <cstdint>

#include "libtorrent/config.hpp"

#include <libtorrent/hex.hpp>
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/aux_/time.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/socket_io.hpp"
#include "libtorrent/invariant_check.hpp"
#include "libtorrent/address.hpp"

using namespace std::placeholders;

namespace libtorrent { namespace nsw
{
namespace
{
	template <typename T, typename K>
	void erase_one(T& container, K const& key)
	{
		typename T::iterator i = container.find(key);
		TORRENT_ASSERT(i != container.end());
		container.erase(i);
	}

	bool verify_node_address(nsw_settings const& settings
		, node_id const& id, address const& addr)
	{
		return false;
	}
}

void ip_set::insert(address const& addr)
{

}

bool ip_set::exists(address const& addr) const
{
	return false;
}

void ip_set::erase(address const& addr)
{

}

routing_table::routing_table(node_id const& id, udp proto, int bucket_size
	, nsw_settings const& settings
	, nsw_logger_interface* log)
	:
#ifndef TORRENT_DISABLE_LOGGING
	m_log(log),
#endif
	m_settings(settings)
	, m_id(id)
	, m_protocol(proto)
	, m_depth(0)
	, m_last_self_refresh(min_time())
	, m_bucket_size(bucket_size)
{
	TORRENT_UNUSED(log);
	m_buckets.reserve(30);
}

int routing_table::bucket_limit(int bucket) const
{
	return 0;
}

void routing_table::status(std::vector<nsw_routing_bucket>& s) const
{

}

#ifndef TORRENT_NO_DEPRECATE
void routing_table::status(session_status& s) const
{

}
#endif

std::tuple<int, int, int> routing_table::size() const
{
}

std::int64_t routing_table::num_global_nodes() const
{
	return 0;
}

int routing_table::depth() const
{
	return m_depth;
}

node_entry const* routing_table::next_refresh()
{
	node_entry* candidate = nullptr;

	return candidate;
}

void routing_table::replacement_cache(bucket_t& nodes) const
{

}

routing_table::table_t::iterator routing_table::find_bucket(node_id const& id)
{
	table_t::iterator i = m_buckets.begin();
	return i;
}


bool compare_ip_cidr(address const& lhs, address const& rhs)
{
	return false;
}

node_entry* routing_table::find_node(udp::endpoint const& ep
	, routing_table::table_t::iterator* bucket)
{

	return nullptr;
}

void routing_table::fill_from_replacements(table_t::iterator bucket)
{

}

void routing_table::remove_node_internal(node_entry* n, bucket_t& b)
{

}

void routing_table::remove_node(node_entry* n
	, routing_table::table_t::iterator bucket)
{

}

bool routing_table::add_node(node_entry const& e)
{
	return false;
}

routing_table::add_node_status_t routing_table::add_node_impl(node_entry e)
{
	return failed_to_add;
}

void routing_table::split_bucket()
{

}

void routing_table::update_node_id(node_id const& id)
{

}

void routing_table::for_each_node(
	void (*fun1)(void*, node_entry const&)
	, void (*fun2)(void*, node_entry const&)
	, void* userdata) const
{

}

void routing_table::node_failed(node_id const& nid, udp::endpoint const& ep)
{

}

void routing_table::add_router_node(udp::endpoint const& router)
{

}


void routing_table::heard_about(node_id const& id, udp::endpoint const& ep)
{

}

bool routing_table::node_seen(node_id const& id, udp::endpoint const& ep, int rtt)
{
	return false;
}

void routing_table::find_node(node_id const& target
	, std::vector<node_entry>& l, int const options, int count)
{

}

#if TORRENT_USE_INVARIANT_CHECKS
void routing_table::check_invariant() const
{

}
#endif

bool routing_table::is_full(int const bucket) const
{
	return false;
}

#ifndef TORRENT_DISABLE_LOGGING
void routing_table::log_node_failed(node_id const& nid, node_entry const& ne) const
{

}
#endif

} } // namespace libtorrent::nsw
