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
	// This structure helps to store and load the state
	// of the ``nsw_tracker``.
	// supports both IPv4 and IPv6 at the same time for
	class TORRENT_EXPORT nsw_state
	{
	public:
		// the id of the IPv4 node
		node_id nid;
#if TORRENT_USE_IPV6
		// the id of the IPv6 node
		node_id nid6;
#endif
		// the bootstrap nodes saved from the IPv4 buckets node
		std::vector<udp::endpoint> nodes;
#if TORRENT_USE_IPV6
		// the bootstrap nodes saved from the IPv6 buckets node
		std::vector<udp::endpoint> nodes6;
#endif
		void clear();

		TORRENT_EXTRA_EXPORT void read_nsw_state(bdecode_node const& e,nsw_state& ret_state);
		TORRENT_EXTRA_EXPORT entry save_nsw_state(nsw_state const& state);
	private:
		node_id extract_node_id(bdecode_node const& e, string_view key);
		entry save_nodes(std::vector<udp::endpoint> const& nodes);
	};

}}

#endif // LIBTORRENT_NSW_STATE_HPP
