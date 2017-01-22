#ifndef TORRENT_NSW_TRACKER_HPP
#define TORRENT_NSW_TRACKER_HPP

#include <functional>

#include "libtorrent/navigable_small_world/node.hpp"
#include <libtorrent/navigable_small_world/nsw_state.hpp>

#include <libtorrent/socket.hpp>
#include <libtorrent/deadline_timer.hpp>
#include <libtorrent/span.hpp>
#include <libtorrent/io_service.hpp>


namespace libtorrent
{
	struct counters;
	struct nsw_settings;
#ifndef TORRENT_NO_DEPRECATE
	struct session_status;
#endif
}

namespace libtorrent { namespace nsw
{

struct TORRENT_EXTRA_EXPORT nsw_tracker final
		: udp_socket_interface
		, std::enable_shared_from_this<nsw_tracker>
	{
		using send_fun_t = std::function<void(udp::endpoint const&
			, span<char const>, error_code&, int)>;

		nsw_tracker(nsw_observer* observer
			, io_service& ios
			, send_fun_t const& send_fun
			, nsw_settings const& settings
			, counters& cnt
			, nsw_storage_interface& storage
			, nsw_state state);
		virtual ~nsw_tracker();

		//void start(find_data::nodes_callback const& f);
		void start();
		void stop();

		void update_node_id();

		void add_node(udp::endpoint const& node);
		void add_router_node(udp::endpoint const& node);

		nsw_state state() const;

		enum flags_t { flag_seed = 1, flag_implied_port = 2 };
		void get_peers(sha1_hash const& ih
			, std::function<void(std::vector<tcp::endpoint> const&)> f);
		void announce(sha1_hash const& ih, int listen_port, int flags
			, std::function<void(std::vector<tcp::endpoint> const&)> f);

		void get_item(sha1_hash const& target
			, std::function<void(item const&)> cb);

		void get_item(public_key const& key
			, std::function<void(item const&, bool)> cb
			, std::string salt = std::string());

		void put_item(entry const& data
			, std::function<void(int)> cb);

		void put_item(public_key const& key
			, std::function<void(item const&, int)> cb
			, std::function<void(item&)> data_cb, std::string salt = std::string());

		void direct_request(udp::endpoint const& ep, entry& e
			, std::function<void(msg const&)> f);

#ifndef TORRENT_NO_DEPRECATE
		void nsw_status(session_status& s);
#endif
		void nsw_status(std::vector<nsw_routing_bucket>& table
			, std::vector<nsw_lookup>& requests);
		void update_stats_counters(counters& c) const;

		void incoming_error(error_code const& ec, udp::endpoint const& ep);
		bool incoming_packet(udp::endpoint const& ep, span<char const> buf);

	private:

		std::shared_ptr<nsw_tracker> self()
		{ return shared_from_this(); }

		void connection_timeout(node& n, error_code const& e);
		void refresh_timeout(error_code const& e);
		void refresh_key(error_code const& e);
		void update_storage_node_ids();

		virtual bool has_quota() override;
		virtual bool send_packet(entry& e, udp::endpoint const& addr) override;

		bdecode_node m_msg;

		counters& m_counters;
		nsw_storage_interface& m_storage;
		nsw_state m_state; // to be used only once
		node m_nsw;
#if TORRENT_USE_IPV6
		node m_nsw6;
#endif
		send_fun_t m_send_fun;
		nsw_logger* m_log;

		std::vector<char> m_send_buf;

		deadline_timer m_key_refresh_timer;
		deadline_timer m_connection_timer;
#if TORRENT_USE_IPV6
		deadline_timer m_connection_timer6;
#endif
		deadline_timer m_refresh_timer;
		nsw_settings const& m_settings;

		std::map<std::string, node*> m_nodes;

		bool m_abort;

		udp::resolver m_host_resolver;


		time_point m_last_tick;
	};
}}

#endif //TORRENT_NSW_TRACKER_HPP
