#ifndef LIBTORRENT_NSW_STATE_HPP
#define LIBTORRENT_NSW_STATE_HPP

#include <libtorrent/config.hpp>
#include <libtorrent/socket.hpp>
#include <libtorrent/entry.hpp>

#include <libtorrent/navigable_small_world/node_id.hpp>

#include <vector>

namespace libtorrent
{
	struct bdecode_node;
}

namespace libtorrent {
namespace nsw
{
	struct TORRENT_EXPORT nsw_state
	{
		node_id nid;
		node_id nid6;

		std::vector<udp::endpoint> nodes;
		std::vector<udp::endpoint> nodes6;

		void clear();
	};

	TORRENT_EXTRA_EXPORT nsw_state read_nsw_state(bdecode_node const& e);
	TORRENT_EXTRA_EXPORT entry save_nsw_state(nsw_state const& state);
}}

#endif // LIBTORRENT_NSW_STATE_HPP
