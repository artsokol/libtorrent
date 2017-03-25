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
#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/aux_/time.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/socket_io.hpp"
#include "libtorrent/address.hpp"

using namespace std::placeholders;

namespace libtorrent { namespace nsw
{
namespace
{

	bool verify_node_address(nsw_settings const& settings
		, node_id const& id, address const& addr)
	{
		return verify_id(id, addr);
	}
}


routing_table::routing_table(node_id const& id, udp proto, int neighbourhood_size
	, nsw_settings const& settings
	, std::string const& node_description
	, nsw_logger_interface* log)
	: m_neighbourhood_size(neighbourhood_size)
	, m_equal_text_max_nodes(5)
	, m_settings(settings)
	, m_description(node_description)
	, m_id(id)
	, m_protocol(proto)
	, m_last_self_refresh(min_time())
//	, m_ips(0)
	, m_close_nodes_rt(neighbourhood_size)
	, m_far_nodes_rt(neighbourhood_size)
	, m_replacement_nodes(m_description)
#ifndef TORRENT_DISABLE_LOGGING
	, m_log(log)
#endif
{
//	TORRENT_UNUSED(log);
}

void routing_table::status(nsw_routing_info& s) const
{
	s.num_nodes = m_close_nodes_rt.size();
	s.num_long_links = m_far_nodes_rt.size();
}


std::tuple<size_t, size_t, size_t> routing_table::size() const
{
	size_t friend_nodes = m_close_nodes_rt.size();
	size_t old_friend_nodes = m_far_nodes_rt.size();
	size_t confirmed = 0;

	for (auto const& k : m_close_nodes_rt)
	{
		if (k.confirmed()) ++confirmed;
	}

	for (auto const& k : m_far_nodes_rt)
	{
		if (k.confirmed()) ++confirmed;
	}

	return std::make_tuple(friend_nodes, old_friend_nodes, confirmed);
}

// concerns about goals !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
node_entry const* routing_table::next_refresh()
{
	node_entry* candidate = nullptr;

	//for (auto const &i : m_close_nodes_rt)
	for (routing_table_t::iterator i = m_close_nodes_rt.begin();
		i != m_close_nodes_rt.end(); ++i)
	{
		if (i->id == m_id) continue;

		if (i->last_queried == min_time())
		{
			candidate = &*i;
			break;
			//goto out;
		}

		if (candidate == nullptr || i->last_queried < candidate->last_queried)
		{
			candidate = &*i;
		}
	}
	// make sure we don't pick the same node again next time we want to refresh
	// the routing table
	if (candidate)
		candidate->last_queried = aux::time_now();

	return candidate;
}

node_entry* routing_table::find_node(udp::endpoint const& ep
									, routing_table::table_type_t& type
									, int& index)
{
	for (routing_table_t::iterator j = m_close_nodes_rt.begin();
		j != m_close_nodes_rt.end(); ++j)
	{
		if (j->addr() != ep.address() ||
			j->port() != ep.port())
		{
			continue;
		}
		index = j - m_close_nodes_rt.begin();
		type = routing_table::closest_nodes;
		return &*j;
	}

	for (routing_table_t::iterator j = m_far_nodes_rt.begin();
		j != m_far_nodes_rt.end(); ++j)
	{
		if (j->addr() != ep.address() ||
			j->port() != ep.port())
		{
			continue;
		}
		index = j - m_far_nodes_rt.begin();
		type = routing_table::far_nodes;
		return &*j;
	}
	type = routing_table::none;
	index = -1;
	return nullptr;
}

void routing_table::remove_node(node_entry* n, routing_table_t& rt)
{
	if (!rt.empty()
		&& n >= &rt[0]
		&& n < &rt[0] + rt.size())
	{
		std::ptrdiff_t const idx = n - &rt[0];
		rt.erase(rt.begin() + idx);

		std::pair <replacement_table_t::iterator,replacement_table_t::iterator> ret;
		ret = m_replacement_nodes.equal_range(n->term_vector);
		m_replacement_nodes.erase(ret.first,ret.second);
	}
}

bool routing_table::add_node(node_entry const& e)
{
	add_node_status_t s = add_node_impl(e);
	if (s == failed_to_add) return false;
	else if(s == node_added) return true;
}

routing_table::add_node_status_t routing_table::add_node_impl(node_entry e)
{
	int index = -1;
	routing_table_t::iterator j;
	routing_table::table_type_t table_type = routing_table::none;
	// don't add if the address isn't the right type
	if (!is_native_endpoint(e.endpoint))
		return failed_to_add;

	// don't add ourself
	if (e.id == m_id) return failed_to_add;

	// check if we already have this node
	node_entry* existing = find_node(e.endpoint, table_type, index);
	if (existing != nullptr)
	{

		// found node, possibly this is node from our list
		// with changed term vector
		if(e.id == existing->id &&
			e.term_vector == existing->term_vector)
		{
			// no
			existing->timeout_count = 0;
			existing->update_rtt(e.rtt);
			if (e.pinged())
			{
				existing->update_rtt(e.rtt);
				existing->last_queried = e.last_queried;
			}
			return node_exists;
		}

	}

	if( term_vector::getSimilarity(m_close_nodes_rt.back().term_vector,m_description) >
		term_vector::getSimilarity(e.term_vector,m_description))
	{
		// in common we should not include this node in our friend list
		// but there ase several cases when it can be useful. Let's check

		// if there is no room, we look for nodes that are not 'pinged',
		// i.e. we haven't confirmed that they respond to messages.
		// Then we look for nodes marked as stale
		// in the k-bucket. If we find one, we can replace it.
		// then we look for nodes with the same 3 bit prefix (or however
		// many bits prefix the bucket size warrants). If there is no other
		// node with this prefix, remove the duplicate with the highest RTT.
		// as the last replacement strategy, if the node we found matching our
		// bit prefix has higher RTT than the new node, replace it.

		if (e.pinged() && e.fail_count() == 0
			&& m_replacement_nodes.find(e.term_vector) == m_replacement_nodes.end())
		{
			// if the node we're trying to insert is considered pinged,
			// we may replace other nodes that aren't pinged
			routing_table_t::reverse_iterator k = std::find_if(m_close_nodes_rt.rbegin(),
															m_close_nodes_rt.rend(),
															[](node_entry const& ne)
															{ return ne.pinged() == false; });

			if (k != m_close_nodes_rt.rend() && !k->pinged())
			{
				// j points to a node that has not been pinged.
				// Replace it with this new one
				*k = e;
				sort(m_close_nodes_rt.begin(), m_close_nodes_rt.end(),
					[&e](node_entry const& a,node_entry const& b) -> bool
					{
						return term_vector::getSimilarity(a.term_vector,e.term_vector) >
									term_vector::getSimilarity(b.term_vector,e.term_vector);
					});

#ifndef TORRENT_DISABLE_LOGGING
				log_node_added(*k);
#endif
				return node_added;
			}

			// A node is considered stale if it has failed at least one
			// time. Here we choose the node that has failed most times.
			// If we don't find one, place this node in the replacement-
			// cache and replace any nodes that will fail in the future
			// with nodes from that cache.

			// j = std::max_element(m_close_nodes_rt.begin(), m_close_nodes_rt.end()
			// 	, [](node_entry const& lhs, node_entry const& rhs)
			// 	{ return lhs.fail_count() < rhs.fail_count(); });

			// if (j->fail_count() > 0)
			// {
			// 	*j = e;
			// 	sort(m_close_nodes_rt);
			// 	return node_added;
			// }

		}
		return failed_to_add;
	} // getSimilarity

	// looking for other node with the same term_vector
	j = std::find_if(m_close_nodes_rt.begin()
					, m_close_nodes_rt.end()
					, [&e](node_entry const& ne)
					{ return ne.term_vector == e.term_vector; });

	if (j == m_close_nodes_rt.end())
	{
		// check if it exist in vector of far nodes
		j = std::find_if(m_far_nodes_rt.begin()
						, m_far_nodes_rt.end()
						, [&e](node_entry const& ne)
						{ return ne.term_vector == e.term_vector; });

		if (j != m_far_nodes_rt.end())
		{
			j->timeout_count = 0;
			j->update_rtt(e.rtt);
			remove_node(&*j,m_far_nodes_rt);
			return insert_node(*j);
		}

		return insert_node(e);
	}
	else
	{
		// replacement cache inserting
		// but we say fsiled as it is not included in close list
		m_replacement_nodes.emplace(e.term_vector,e);
	}
	return failed_to_add;
}

routing_table::add_node_status_t routing_table::insert_node(const node_entry& e)
{
	int index = -1;
	routing_table::table_type_t table_type = routing_table::none;

	node_entry* existing = find_node(e.endpoint,table_type,index);

	if (existing != nullptr &&
		table_type == routing_table::closest_nodes)
	{
		remove_node(existing,m_close_nodes_rt);
		m_close_nodes_rt.push_back(e);

		sort(m_close_nodes_rt.begin(), m_close_nodes_rt.end(),
					[&e](node_entry const& a,node_entry const& b) -> bool
					{
						return term_vector::getSimilarity(a.term_vector,e.term_vector) >
										term_vector::getSimilarity(b.term_vector,e.term_vector);
					});
#ifndef TORRENT_DISABLE_LOGGING
		log_node_added(e);
#endif
		return node_added;
	}
	else if (table_type == routing_table::far_nodes)
	{
		// node from far list has chenged it's description
		remove_node(existing,m_far_nodes_rt);
	}

	//node_entry& last_in_closest = m_close_nodes_rt.back();
	m_far_nodes_rt.push_back(m_close_nodes_rt.back());
	m_close_nodes_rt.pop_back();
	m_close_nodes_rt.push_back(e);

	sort(m_close_nodes_rt.begin(), m_close_nodes_rt.end(),
				[&e](node_entry const& a,node_entry const& b) -> bool
				{
					return term_vector::getSimilarity(a.term_vector,e.term_vector) >
										term_vector::getSimilarity(b.term_vector,e.term_vector);
				});

	sort(m_far_nodes_rt.begin(), m_close_nodes_rt.end(),
				[&e](node_entry const& a,node_entry const& b) -> bool
				{
					return term_vector::getSimilarity(a.term_vector,e.term_vector) >
										term_vector::getSimilarity(b.term_vector,e.term_vector);
				});

#ifndef TORRENT_DISABLE_LOGGING
	log_node_added(e);
#endif

	return node_added;


}

void routing_table::update_node_id(node_id const& id)
{
	m_id = id;

	// pull all nodes out of the routing table, effectively emptying it
	routing_table_t old_close_nodes;
	routing_table_t old_far_nodes;

	old_close_nodes.swap(m_close_nodes_rt);
	old_far_nodes.swap(m_far_nodes_rt);

	// then add them all back. First add the main nodes, then the replacement
	// nodes
	for (auto const& n : old_close_nodes)
		add_node(n);

	// now add back the replacement nodes
	for (auto const& n : old_far_nodes)
		add_node(n);
}

void routing_table::for_each_node(
	void (*func_for_close_nodes)(void*, node_entry const&)
	, void (*func_for_far_nodes)(void*, node_entry const&)
	, void* userdata) const
{
	if (func_for_close_nodes)
	{
		for (auto const& j : m_close_nodes_rt)
			func_for_close_nodes(userdata, j);
	}

	if (func_for_far_nodes)
	{
		for (auto const& j : m_far_nodes_rt)
			func_for_far_nodes(userdata, j);
	}
}

bool routing_table::fill_from_replacements(node_entry& ne)
{
	std::pair <replacement_table_t::iterator,replacement_table_t::iterator> ret;
	ret = m_replacement_nodes.equal_range(ne.term_vector);

	if(ret.first != ret.second)
	{
		routing_table_t tmp_rt;

		for (replacement_table_t::iterator it=ret.first;
		 							it!=ret.second;
		 							++it)
		{
			tmp_rt.push_back(it->second);
			std::sort(tmp_rt.begin(), tmp_rt.end()
					, [](node_entry const& lhs, node_entry const& rhs)
						{ return lhs.rtt < rhs.rtt; });
		}

		//remove_node(&ne,m_close_nodes_rt);
		ne = tmp_rt.front();
		//add_node(node_to_replace);

		//remove from cache;
		replacement_table_t::iterator point_to_remove = std::find_if(ret.first
														, ret.second
												, [&ne](const replacement_table_t::value_type& map_item)
												{ return map_item.second.id == ne.id; });

		m_replacement_nodes.erase(point_to_remove);

		return true;
	}
	return false;
}

void routing_table::node_failed(node_id const& nid, udp::endpoint const& ep)
{
	// if messages to ourself fails, ignore it
	if (nid == m_id) return;

	routing_table_t::iterator j = std::find_if(m_close_nodes_rt.begin()
												, m_close_nodes_rt.end()
												, [&nid](node_entry const& ne)
												{ return ne.id == nid; });

	if (j == m_close_nodes_rt.end())
	{
		node_entry* found_node = nullptr;
		replacement_table_t::iterator k;

		j = std::find_if(m_far_nodes_rt.begin()
						, m_far_nodes_rt.end()
						, [&nid](node_entry const& ne)
						{ return ne.id == nid; });

		if (j == m_far_nodes_rt.end()
			/*|| j->endpoint != ep*/)
		{
			// let's check replacemet list
			k = std::find_if(m_replacement_nodes.begin()
							, m_replacement_nodes.end()
							, [&nid](const replacement_table_t::value_type& map_item)
							{ return map_item.second.id == nid; });

			if (k == m_replacement_nodes.end())
				return;
			else
				found_node	= &(k->second);
		}
		else
			found_node = &*j;

		found_node->timed_out();

#ifndef TORRENT_DISABLE_LOGGING
		log_node_failed(nid, *found_node);
#endif
		// if this node has failed too many times, or if this node
		// has never responded at all, remove it
		if (found_node->fail_count() >= m_settings.max_fail_count || !found_node->pinged())
		{
			//m_ips.erase(j->addr());
			if (j == m_far_nodes_rt.end())
				m_far_nodes_rt.erase(j);
			else
				m_replacement_nodes.erase(k);


		}

		return;
	}

	if(j->endpoint != ep)
	{
		// if the endpoint doesn't match, it's a different node
		// claiming the same ID. The node we have in our routing
		// table is not necessarily stale
		return;
	}


	j->timed_out();

#ifndef TORRENT_DISABLE_LOGGING
	log_node_failed(nid, *j);
#endif

	// if this node has failed too many times, or if this node
	// has never responded at all, remove it
	if (j->fail_count() >= m_settings.max_fail_count || !j->pinged())
	{
		// we try to swap failed node with new one with the
		// same text from replacemet nodes
		if (!fill_from_replacements(*j))
		{
			// have not found
			remove_node(&*j,m_close_nodes_rt);

			// TODO:
			//new_one = get_node
			//add_node(new_one)
		}
	}

}

void routing_table::add_router_node(udp::endpoint const& router)
{
	m_router_nodes.insert(router);
}

bool routing_table::node_seen(node_id const& id, udp::endpoint const& ep, int rtt)
{
 	return verify_node_address(m_settings, id, ep.address()) && add_node(node_entry(id, ep, rtt, true));
}

bool routing_table::is_full() const
{
	int num_close_nodes = int(m_close_nodes_rt.size());
	if (num_close_nodes == 0 ||
		m_neighbourhood_size > num_close_nodes)
		return false;

	return true;

}

#ifndef TORRENT_DISABLE_LOGGING
void routing_table::log_node_failed(node_id const& nid, node_entry const& ne) const
{
	if (m_log != nullptr && m_log->should_log(nsw_logger_interface::routing_table))
	{
		m_log->log(nsw_logger_interface::routing_table, "NODE FAILED id: %s \
														fails: %d \
														pinged: %d \
														up-time: %d"
			, aux::to_hex(nid).c_str()
			, ne.fail_count()
			, int(ne.pinged())
			, int(total_seconds(aux::time_now() - ne.first_seen)));
	}
}

void routing_table::log_node_added(node_entry const& ne) const
{
	if (m_log != nullptr && m_log->should_log(nsw_logger_interface::routing_table))
	{
		m_log->log(nsw_logger_interface::routing_table, "NODE ADDED id: %s \
														up-time: %d\n \
														description %s"
			, aux::to_hex(ne.id).c_str()
			, int(total_seconds(aux::time_now() - ne.first_seen))
			, ne.term_vector.c_str());
	}
}
#endif


// std::int64_t routing_table::num_global_nodes() const
// {
// 	return 0;
// }

// int routing_table::depth() const
// {
// 	return m_depth;
// }
// void routing_table::replacement_cache(bucket_t& nodes) const
// {

// }

// routing_table::table_t::iterator routing_table::find_bucket(node_id const& id)
// {
// 	table_t::iterator i = m_buckets.begin();
// 	return i;
// }


// bool compare_ip_cidr(address const& lhs, address const& rhs)
// {
// 	return false;
// }
// void routing_table::remove_node_internal(node_entry* n, routing_table_t& rt)
// {

// }
// void routing_table::split_bucket()
// {

// }

// void routing_table::heard_about(node_id const& id, udp::endpoint const& ep)
// {
// 	if (!verify_node_address(m_settings, id, ep.address()))
// 		return;
// 	add_node(node_entry(id, ep));
// }
// void routing_table::find_node(node_id const& target
// 	, std::vector<node_entry>& l, int const options, int count)
// {

// }
} } // namespace libtorrent::nsw
