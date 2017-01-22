#ifndef NSW_OBSERVER_HPP
#define NSW_OBSERVER_HPP

#include "libtorrent/string_view.hpp"
#include "libtorrent/config.hpp"
#include "libtorrent/address.hpp"
#include "libtorrent/navigable_small_world/msg.hpp"
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/entry.hpp"

namespace libtorrent { namespace nsw
{
	struct TORRENT_EXTRA_EXPORT nsw_logger
	{
#ifndef TORRENT_DISABLE_LOGGING
		enum module_t
		{
			tracker,
			node,
			routing_table,
			rpc_manager,
			traversal
		};

		enum message_direction_t
		{
			incoming_message,
			outgoing_message
		};

		virtual bool should_log(module_t m) const = 0;
		virtual void log(module_t m, char const* fmt, ...) TORRENT_FORMAT(3,4) = 0;
		virtual void log_packet(message_direction_t dir, span<char const> pkt
			, udp::endpoint const& node) = 0;
#endif

	protected:
		~nsw_logger() {}
	};

	struct TORRENT_EXTRA_EXPORT nsw_observer : nsw_logger
	{
		virtual void set_external_address(address const& addr
			, address const& source) = 0;
		virtual address external_address(udp proto) = 0;
		virtual void get_peers(sha1_hash const& ih) = 0;
		virtual void outgoing_get_peers(sha1_hash const& target
			, sha1_hash const& sent_target, udp::endpoint const& ep) = 0;
		virtual void announce(sha1_hash const& ih, address const& addr, int port) = 0;
		virtual bool on_nsw_request(string_view query
			, nsw::msg const& request, entry& response) = 0;

	protected:
		~nsw_observer() {}
	};
}}

#endif // NSW_OBSERVER_HPP
