#ifndef NSW_NODE_HPP
#define NSW_NODE_HPP

#include <map>
#include <set>
#include <mutex>
#include <cstdint>
#include <iomanip> // setprecision
#include <iostream>

#include "libtorrent/config.hpp"
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"
#include "libtorrent/navigable_small_world/find_data.hpp"

#include "libtorrent/socket.hpp"
#include "libtorrent/string_view.hpp"
#include "libtorrent/alert_types.hpp"

using namespace std::placeholders;

namespace libtorrent {
	struct counters;
	struct nsw_settings;
}

namespace libtorrent { namespace nsw
{

struct traversal_algorithm;
struct nsw_logger_observer_interface;
struct nsw_tracker;
struct msg;

TORRENT_EXTRA_EXPORT entry write_nodes_entry(std::vector<node_entry> const& nodes
												, vector_t const& requested_vec
												, entry& e
												, int lvl
												, int lay);

class add_friend_observer : public observer_interface
{
public:
	add_friend_observer(std::shared_ptr<traversal_algorithm> const& algo
		, udp::endpoint const& ep
		, node_id const& id
		, vector_t const& text)
		: observer_interface(algo, ep, id, text)
	{}

	void reply(libtorrent::nsw::msg const&);
};

class search_query_observer : public observer_interface
{
public:
	search_query_observer(std::shared_ptr<traversal_algorithm> const& algo
		, udp::endpoint const& ep
		, node_id const& id
		, vector_t const& text)
		: observer_interface(algo, ep, id, text)
	{}
	void reply(libtorrent::nsw::msg const&);
};


struct udp_socket_interface
{
//	virtual bool has_quota() = 0;
	virtual bool send_packet(entry& e, udp::endpoint const& addr) = 0;
protected:
	~udp_socket_interface() {}
};

class TORRENT_EXTRA_EXPORT node : boost::noncopyable
{

private:

	libtorrent::nsw_settings const& m_settings;

	std::mutex m_mutex;

	std::set<traversal_algorithm*> m_running_requests;

	node_id m_id;
	std::string m_description;

	//usefull to have a precomputed vector
	// create in routing table
	//std::unordered_map<std::string, double> m_vector_description;
public:
	nsw::routing_table m_table;
	rpc_manager m_rpc;

private:

	struct protocol_descriptor
	{
		udp protocol;
		char const* family_name;
		char const* nodes_key;
	};

	static protocol_descriptor const& map_protocol_to_descriptor(udp protocol);

//	std::map<std::string, node*> const& m_nodes;

	nsw_logger_observer_interface* m_observer;

	protocol_descriptor const& m_protocol;

	time_point m_last_tracker_tick;

	time_point m_last_self_refresh;

	int m_secret[2];

	udp_socket_interface* m_sock;
	counters& m_counters;

	std::map<double, std::string, std::greater<double> > m_query_results;
	nsw_lookup m_last_query_stat;
	uint16_t m_search_routines;
	bool m_is_query_running;

public:
	node(udp proto, udp_socket_interface* sock
		, libtorrent::nsw_settings const& settings
		, node_id const& nid
		, const std::string& description
		, nsw_logger_observer_interface* observer, counters& cnt);

	~node();

	void update_node_id();
	void update_node_description(std::string const& new_descr);
	void tick();
	void bootstrap(find_data::nodes_callback const& f);

	void add_gate_node(udp::endpoint const& router
					, vector_t const& description);
	void nsw_query(vector_t& query_vec);
	void handle_search_query_response(std::string const& description_str);
	void unreachable(udp::endpoint const& ep);
	void incoming(msg const& m);

	node_id const& nid() const { return m_id; }
	std::string const& descr() const { return m_description; }
	vector_t const& descr_vec() const { return m_table.get_descr(); }
#ifndef TORRENT_DISABLE_LOGGING
	std::uint32_t search_id() { return m_search_id++; }
#endif

	std::tuple<int, /*int,*/ int> size() const { return m_table.size(); }

	// std::int64_t num_global_nodes() const
	// { return m_table.num_global_nodes(); }

	enum flags_t { flag_seed = 1, flag_implied_port = 2 };
	// void get_friends(sha1_hash const& info_hash
	// 	, vector_t const& target
	// 	, std::function<void(std::vector<std::tuple<node_id, udp::endpoint, std::string, double>> const&)> dcallback
	// 	, std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> ncallback);
	void add_friend(sha1_hash const& info_hash, int listen_port, int flags
		, std::function<void(std::vector<tcp::endpoint> const&)> f);

	// void direct_request(udp::endpoint const& ep, entry& e
	// 	, std::function<void(msg const&)> f);

	bool verify_token(string_view token, sha1_hash const& info_hash
	 , udp::endpoint const& addr) const;

 	std::string generate_token(udp::endpoint const& addr, sha1_hash const& info_hash);

	time_duration connection_timeout();

	void new_write_key();

	// void add_node(udp::endpoint const& node, node_id const& id);
	// to doublecheck!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// void replacement_cache(routing_table_t& nodes) const
	// { m_table.replacement_cache(nodes); }

	int get_search_threads() const { return m_settings.search_threads; }

	void add_traversal_algorithm(traversal_algorithm* a)
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_running_requests.insert(a);
	}

	void remove_traversal_algorithm(traversal_algorithm* a)
	{
		std::lock_guard<std::mutex> l(m_mutex);
		m_running_requests.erase(a);
	}

	void status(std::vector<std::pair<int, std::vector<node_entry>>>& cf_table);

	nsw_lookup const& get_stat(){return m_last_query_stat;};
	std::tuple<int, int, int> get_stats_counters() const;

	libtorrent::nsw_settings const& settings() const { return m_settings; }
	counters& stats_counters() const { return m_counters; }

	nsw_logger_observer_interface* observer() const { return m_observer; }

	udp get_protocol() const { return m_protocol.protocol; }
	char const* get_protocol_family_name() const { return m_protocol.family_name; }
	//char const* get_protocol_nodes_key() const { return m_protocol.nodes_key; }

	bool native_address(udp::endpoint const& ep) const
	{ return ep.protocol().family() == m_protocol.protocol.family(); }

	bool native_address(tcp::endpoint const& ep) const
	{ return ep.protocol().family() == m_protocol.protocol.family(); }

	bool native_address(address const& addr) const
	{
		return (addr.is_v4() && m_protocol.protocol == m_protocol.protocol.v4())
			|| (addr.is_v6() && m_protocol.protocol == m_protocol.protocol.v6());
	}
	void handle_nsw_query_result(std::string& result, double similarity_with_query);

	void get_query_result(std::vector<std::string>& results_copy);
	void dump_query_result(std::ofstream& dump_file);
private:
	void send_ping(udp::endpoint const& ep/*, int bucket*/
		, node_id const& id);
	void lookup_friends(sha1_hash const& info_hash
		, vector_t const& target
		, entry& reply
		, address const& requester) const;
	void incoming_request(msg const& h, entry& e);
	void write_nodes_entries(bdecode_node const& want, entry& r, int lvl, int lay);

public:
	void nsw_query_engine(traversal_algorithm::callback_data_t const& v, nsw_lookup const& stat);
	void add_friend_engine(traversal_algorithm::callback_data_t const& v, nsw_lookup const& stat);
	boost::unordered::unordered_map<sha1_hash,boost::shared_ptr<nsw::term_vector> > m_torrents_term_vectors;

#ifndef TORRENT_DISABLE_LOGGING
	std::uint32_t m_search_id = 0;
#endif
};


} }

#endif // NSW_NODE_HPP
