#ifndef TORRENT_NSW_TRACKER_HPP
#define TORRENT_NSW_TRACKER_HPP

#include <functional>
#include <mutex>          // std::mutex

#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_state.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"

#include "libtorrent/socket.hpp"
#include "libtorrent/deadline_timer.hpp"
#include "libtorrent/span.hpp"
#include "libtorrent/io_service.hpp"
#include "libtorrent/bdecode.hpp"

namespace libtorrent
{
	struct counters;
	struct nsw_settings;

}

namespace libtorrent { namespace nsw
{

struct TORRENT_EXTRA_EXPORT nsw_tracker final
		: udp_socket_interface
		, std::enable_shared_from_this<nsw_tracker>
	{
		typedef struct timers_t{
			deadline_timer key_refresh_timer;
			deadline_timer connection_timer;
			deadline_timer refresh_timer;

			timers_t(io_service& io_serv)
					:key_refresh_timer(io_serv)
					,connection_timer(io_serv)
					,refresh_timer(io_serv)
			{}
		} timers_t;

		typedef enum {not_init=0,started,aborted} status_t;

		using send_fun_t = std::function<void(udp::endpoint const&
			, span<char const>, error_code&, int)>;
private:
		using node_collection_t = std::map<std::string, std::shared_ptr<node>>;
		bdecode_node m_msg;

		counters& m_counters;
		nsw_state m_state; // to be used only once


		send_fun_t m_send_fun;
		nsw_logger_interface* m_log;
		//nsw_logger_observer_interface* m_observer;
		std::vector<char> m_send_buf;

		std::unordered_map<node_id,std::shared_ptr<timers_t>> m_timers_vec;
		nsw_settings const& m_settings;

		node_collection_t m_nodes;
		// these are used when starting new node
		std::vector<std::pair<vector_t, udp::endpoint>> m_nsw_gate_nodes;
		status_t m_status;

		time_point m_last_tick;
		io_service& m_io_srv;
		const find_data::nodes_callback* m_current_callback;

		std::mutex m_query_processing_mutex;

public:


		nsw_tracker(nsw_logger_observer_interface* observer
			, io_service& ios
			, send_fun_t const& send_fun
			, nsw_settings const& settings
			, counters& cnt
			, nsw_state state);
		virtual ~nsw_tracker();

		void start(find_data::nodes_callback const& f);
		void stop();

		void update_node_id();
		void update_node_description(node& item, std::string const& new_descr);

		void add_node(sha1_hash const& nid, std::string const& description);
		void add_gate_node(udp::endpoint const& node, std::string const& description);
		void nsw_query(std::string const&  query);
		void handle_query_results(std::shared_ptr<std::vector<std::string> > out, unsigned int count);
		nsw_state state() const;

		enum flags_t { flag_seed = 1, flag_implied_port = 2 };
		void get_friends(node& item
			, sha1_hash const& ih
			, std::string const& target
			, std::function<void(std::vector<std::tuple<node_id, udp::endpoint, std::string, double>> const&)> f);
		// void announce(sha1_hash const& ih, int listen_port, int flags
		// 	, std::function<void(std::vector<tcp::endpoint> const&)> f);

//		void direct_request(udp::endpoint const& ep, entry& e
//			, std::function<void(msg const&)> f);

		void nsw_status(nsw_nodes_content_table_t& cf_table
		 	, nsw_nodes_content_table_t& ff_table);
//		void update_stats_counters(counters& c) const;

		void incoming_error(error_code const& ec
					, udp::endpoint const& ep);

		bool incoming_packet(udp::endpoint const& ep, span<char const> buf);

	private:

		std::shared_ptr<nsw_tracker> self()
		{ return shared_from_this(); }

		void connection_timeout(node& n, error_code const& e);
		void ping_timeout(node& n, error_code const& e);
		void refresh_key(node& n, error_code const& e);
		void update_storage_node_ids();

		virtual bool send_packet(entry& e, udp::endpoint const& addr) override;

	};
}}

#endif //TORRENT_NSW_TRACKER_HPP
