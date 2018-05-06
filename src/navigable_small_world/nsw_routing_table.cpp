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
	: m_equal_text_max_nodes(5)
	, m_settings(settings)
	, m_id(id)
	, m_protocol(proto)
	, m_last_self_refresh(min_time())
//	, m_ips(0)
	, m_close_nodes_rt()
	, m_replacement_nodes(m_description_vec)
#ifndef TORRENT_DISABLE_LOGGING
	, m_log(log)
#endif
{
	m_description_vec = term_vector::makeTermVector(node_description,m_description_vec);
//	TORRENT_UNUSED(log);
}

void routing_table::status(nsw_routing_info& s) const
{
	s.num_nodes = m_close_nodes_rt.size();
}

void routing_table::status(routing_table_t& cf_table) const
{
	std::for_each(m_close_nodes_rt.begin(),m_close_nodes_rt.end(),[&cf_table](node_entry const& n)
																	{cf_table.push_back(n);});

}

std::tuple<size_t, size_t> routing_table::size() const
{
	size_t friend_nodes = m_close_nodes_rt.size();
	size_t confirmed = 0;

	for (auto const& k : m_close_nodes_rt)
	{
		if (k.confirmed()) ++confirmed;
	}

	return std::make_tuple(friend_nodes, confirmed);
}

// concerns about goals !!!!!!!!!!!!!!!!!!!!!!!!!!!!!
node_entry const* routing_table::next_for_refresh()
{
	node_entry* candidate = nullptr;

	for (routing_table_t::iterator i = m_close_nodes_rt.begin();
		i != m_close_nodes_rt.end(); ++i)
	{
		if (i->id == m_id) continue;

		if (i->last_queried == min_time())
		{
			candidate = &*i;
			break;
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

	type = routing_table::none;
	index = -1;
	return nullptr;
}

node_entry* routing_table::find_node(node_id const& id)
{
	for (routing_table_t::iterator j = m_close_nodes_rt.begin();
		j != m_close_nodes_rt.end(); ++j)
	{
		if (j->id == id)
		{
			return &*j;
		}
	}

	return nullptr;
}

	// fills the vector with the k nodes from our storage that
	// are nearest to the given text.
void routing_table::find_node(vector_t const& target_string
								, std::vector<node_entry>& l
								, int count)
{
	if (count < 1)
		return;

	if(!m_close_nodes_rt.empty())
		l.insert(l.end(),m_close_nodes_rt.begin(),m_close_nodes_rt.end());

	if(l.empty())
		return;

	sort(l.begin(), l.end(),
			[&target_string](node_entry const& a,node_entry const& b) -> bool
					{
						return term_vector::getVecSimilarity(a.term_vector,target_string) >
									term_vector::getVecSimilarity(b.term_vector,target_string);
					});
	if(l.size() > count)
		l.resize(count);
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
		ret = m_replacement_nodes.equal_range(*n);
		m_replacement_nodes.erase(ret.first,ret.second);
	}
}

bool routing_table::add_friend(node_entry const& e)
{
	add_node_status_t s = add_friend_impl(e);
	if (s == failed_to_add) return false;
	else if(s == node_added) return true;
}

routing_table::add_node_status_t routing_table::add_friend_impl(node_entry e)
{
	int index = -1;
	routing_table_t::iterator j;
	routing_table::table_type_t table_type = routing_table::none;
	// don't add if the address isn't the right type
	if (!is_native_endpoint(e.endpoint))
		return failed_to_add;

	// don't add ourself
	if (e.id == m_id) return failed_to_add;

	// this is first item
	if(m_close_nodes_rt.empty())
		return insert_node(e);

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
	//TODO: add checking of size for m_close_nodes_rt

	if( m_close_nodes_rt.size() == m_neighbourhood_size &&
		term_vector::getVecSimilarity(m_close_nodes_rt.back().term_vector,m_description_vec) >
		term_vector::getVecSimilarity(e.term_vector,m_description_vec))
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

		return failed_to_add;
	} // getSimilarity

	// looking for other node with the same term_vector
	j = std::find_if(m_close_nodes_rt.begin()
					, m_close_nodes_rt.end()
					, [&e](node_entry const& ne)
					{ return ne.term_vector == e.term_vector; });

	if (j == m_close_nodes_rt.end())
	{
		return insert_node(e);
	}
	else
	{
		// replacement cache inserting
		// but we say fsiled as it is not included in close list
		m_replacement_nodes.emplace(e);
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
					[this](node_entry const& a,node_entry const& b) -> bool
					{
						return term_vector::getVecSimilarity(a.term_vector,m_description_vec) >
										term_vector::getVecSimilarity(b.term_vector,m_description_vec);
					});
#ifndef TORRENT_DISABLE_LOGGING
		log_node_added(e);
#endif
		return node_added;
	}
	if (m_close_nodes_rt.size() == m_neighbourhood_size)
	{
		m_close_nodes_rt.pop_back();
	}
	m_close_nodes_rt.push_back(e);


	sort(m_close_nodes_rt.begin(), m_close_nodes_rt.end(),
				[this](node_entry const& a,node_entry const& b) -> bool
				{
					return term_vector::getVecSimilarity(a.term_vector,m_description_vec) >
										term_vector::getVecSimilarity(b.term_vector,m_description_vec);
				});

#ifndef TORRENT_DISABLE_LOGGING
	log_node_added(e);
#endif

	return node_added;


}

void routing_table::update_node_id(node_id const& id)
{
	m_id = id;
}

//TODO
void routing_table::update_description()
{
	// pull all nodes out of the routing table, effectively emptying it
	routing_table_t old_close_nodes;

	old_close_nodes.swap(m_close_nodes_rt);

	// then add them all back. First add the main nodes, then the replacement
	// nodes
	for (auto const& n : old_close_nodes)
		add_friend(n);
}

void routing_table::for_each_node(
	void (*func_for_close_nodes)(void*, node_entry const&)
	, void* userdata) const
{
	if (func_for_close_nodes)
	{
		for (auto const& j : m_close_nodes_rt)
			func_for_close_nodes(userdata, j);
	}
}

//try to save our time and find node in our cache
bool routing_table::fill_from_replacements(node_entry& ne)
{
	std::pair <replacement_table_t::iterator,replacement_table_t::iterator> ret;
	ret = m_replacement_nodes.equal_range(ne);

	if(ret.first != ret.second)
	{
		//nodes for replacement found
		routing_table_t tmp_rt;

		for (replacement_table_t::iterator it=ret.first;
		 							it!=ret.second;
		 							++it)
		{
			tmp_rt.push_back(*it);
		}

		std::sort(tmp_rt.begin(), tmp_rt.end()
				, [](node_entry const& lhs, node_entry const& rhs)
					{ return lhs.rtt < rhs.rtt; });

		//remove_node(&ne,m_close_nodes_rt);
		ne = tmp_rt.front();
		//add_node(node_to_replace);

		//remove from cache;
		replacement_table_t::iterator point_to_remove = std::find_if(ret.first
														, ret.second
												, [&ne](const replacement_table_t::value_type& table_item)
												{ return table_item.id == ne.id; });

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
		const node_entry* found_node = nullptr;
		auto k = std::find_if(m_replacement_nodes.begin()
						, m_replacement_nodes.end()
						, [&nid](const replacement_table_t::value_type& table_item)
						{ return table_item.id == nid; });

		if (k == m_replacement_nodes.end())
			return;
		else
			found_node	= &*k;


		found_node->timed_out();

#ifndef TORRENT_DISABLE_LOGGING
		log_node_failed(nid, *found_node);
#endif
		// if this node has failed too many times, or if this node
		// has never responded at all, remove it
		if (found_node->fail_count() >= m_settings.max_fail_count || !found_node->pinged())
		{
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

void routing_table::add_gate_node(udp::endpoint const& router, vector_t const& description)
{
	auto existed_gate = std::find_if(m_gate_nodes.begin(),m_gate_nodes.end(),[&description]
														(std::pair<vector_t, udp::endpoint> const& item)
														{ return term_vector::getVecSimilarity(description,item.first) == 1.0;});
	if (existed_gate == m_gate_nodes.end())
		m_gate_nodes.push_back(std::make_pair(description, router));
}

bool routing_table::node_seen(node_id const& id, vector_t const& vector, udp::endpoint const& ep, int rtt)
{
 	return verify_node_address(m_settings, id, ep.address()) && add_friend(node_entry(id, ep, vector, rtt, true));
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
		m_log->nsw_log(nsw_logger_interface::routing_table, "NODE FAILED id: %s \
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
		m_log->nsw_log(nsw_logger_interface::routing_table, "NODE ADDED id: %s \
														up-time: %d\n"// \
														description %s"
			, aux::to_hex(ne.id).c_str()
			, int(total_seconds(aux::time_now() - ne.first_seen)));
			//, ne.term_vector.c_str());
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

void routing_table::heard_about(node_id const& id)
{
	int index = -1;
	routing_table::table_type_t table_type = routing_table::none;

	// check if we already have this node
	node_entry* existing = find_node(id);

	if(existing)
	{
		existing->last_queried  = aux::time_now();
		existing->reset_fail_count();
	}

	return;
}

} } // namespace libtorrent::nsw
