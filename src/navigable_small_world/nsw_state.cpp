#include "libtorrent/navigable_small_world/nsw_state.hpp"

#include <libtorrent/bdecode.hpp>
#include <libtorrent/socket_io.hpp>

namespace libtorrent {
namespace nsw
{
namespace
{
	node_id extract_node_id(bdecode_node const& e, string_view key)
	{
		auto nid = e.dict_find_string_value(key);
		return node_id(nid);
	}

	entry save_nodes(std::vector<udp::endpoint> const& nodes)
	{
		entry ret;
		return ret;
	}
} // anonymous namespace

	void nsw_state::clear()
	{
	}

	nsw_state read_nsw_state(bdecode_node const& e)
	{
		(void)e;
		nsw_state ret;
		return ret;
	}

	entry save_nsw_state(nsw_state const& state)
	{
		(void)state;
		entry ret;
		return ret;
	}
}}
