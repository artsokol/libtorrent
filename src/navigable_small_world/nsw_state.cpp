#include "libtorrent/navigable_small_world/nsw_state.hpp"

#include "libtorrent/bdecode.hpp"
#include "libtorrent/socket_io.hpp"

namespace libtorrent {
namespace nsw
{
	inline node_id nsw_state::extract_node_id(bdecode_node const& e, string_view key)
	{
		if (e.type() != bdecode_node::dict_t) return node_id();

		auto nid = e.dict_find_string_value(key);

		if (nid.size() != 20) return node_id();

		return node_id(nid);
	}

	// returns entry object with serialized nodes
	entry nsw_state::save_nodes(std::vector<udp::endpoint> const& nodes)
	{
		entry ret(entry::list_t);
		entry::list_type& list = ret.list();

		for (auto const& ep : nodes)
		{
			std::string node;
			std::back_insert_iterator<std::string> out(node);
			detail::write_endpoint(ep, out);
			list.push_back(entry(node));
		}
		return ret;
	}


	inline void nsw_state::clear()
	{
		nid.clear();
		nid6.clear();

		nodes.clear();
		nodes.shrink_to_fit();
		nodes6.clear();
		nodes6.shrink_to_fit();
	}

	void nsw_state::read_nsw_state(bdecode_node const& e, nsw_state& ret)
	{
		if (e.type() != bdecode_node::dict_t) return;

		ret.nid = extract_node_id(e, "node-id");
#if TORRENT_USE_IPV6
		ret.nid6 = extract_node_id(e, "node-id6");
#endif

		if (bdecode_node const nodes = e.dict_find_list("nodes"))
			ret.nodes = detail::read_endpoint_list<udp::endpoint>(nodes);
#if TORRENT_USE_IPV6
		if (bdecode_node const nodes = e.dict_find_list("nodes6"))
			ret.nodes6 = detail::read_endpoint_list<udp::endpoint>(nodes);
#endif

//		return ret;
	}

	entry nsw_state::save_nsw_state(nsw_state const& state)
	{
		entry ret(entry::dictionary_t);
		ret["node-id"] = state.nid.to_string();
		entry const nodes = save_nodes(state.nodes);
		if (!nodes.list().empty()) ret["nodes"] = nodes;
#if TORRENT_USE_IPV6
		ret["node-id6"] = state.nid6.to_string();
		entry const nodes6 = save_nodes(state.nodes6);
		if (!nodes6.list().empty()) ret["nodes6"] = nodes6;
#endif
		return ret;
	}
}}
