#ifndef NSW_LOGGER_OBSERVER_INTERFACE_HPP
#define NSW_LOGGER_OBSERVER_INTERFACE_HPP

#include "libtorrent/string_view.hpp"
#include "libtorrent/config.hpp"
#include "libtorrent/address.hpp"
#include "libtorrent/navigable_small_world/msg.hpp"
#include "libtorrent/sha1_hash.hpp"
#include "libtorrent/entry.hpp"

namespace libtorrent { namespace nsw
{
	struct TORRENT_EXTRA_EXPORT nsw_logger_interface
	{
#ifndef TORRENT_DISABLE_LOGGING
		enum nsw_log_level_t
		{
			tracker,
			node,
			routing_table,
			rpc_manager,
			traversal
		};

		enum nsw_log_message_direction_t
		{
			incoming_message,
			outgoing_message
		};

		virtual bool nsw_should_log(nsw_log_level_t m) const = 0;
		bool should_log(nsw_log_level_t m) const
		{
			return nsw_should_log(m);
		};
		virtual void nsw_log(nsw_log_level_t m, char const* fmt, ...) TORRENT_FORMAT(3,4) = 0;

		virtual void nsw_log_packet(nsw_log_message_direction_t dir, span<char const> pkt
			, udp::endpoint const& node) = 0;
#endif

	protected:
		~nsw_logger_interface() {}
	};

	struct TORRENT_EXTRA_EXPORT nsw_logger_observer_interface : nsw_logger_interface
	{
		virtual void set_nsw_external_address(address const& addr
			, address const& source) = 0;
		virtual address external_nsw_address(udp proto) = 0;
		virtual void get_friends(sha1_hash const& ih, std::string const& target_text) = 0;
		virtual void outgoing_get_friends(sha1_hash const& target
			, std::string const& target_text, udp::endpoint const& ep) = 0;
		//virtual void announce(sha1_hash const& ih, address const& addr, int port) = 0;
//		virtual bool on_add_friend_request(string_view query
//			, nsw::msg const& request, entry& response) = 0;

	protected:
		~nsw_logger_observer_interface() {}
	};
}}

#endif // NSW_LOGGER_OBSERVER_INTERFACE_HPP
