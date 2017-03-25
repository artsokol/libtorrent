#ifndef NSW_NODE_HPP
#define NSW_NODE_HPP

#include <map>
#include <set>
#include <mutex>
#include <cstdint>

#include "libtorrent/config.hpp"
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"

#include "libtorrent/socket.hpp"
#include "libtorrent/string_view.hpp"
#include "libtorrent/alert_types.hpp"


namespace libtorrent {
	struct counters;
//	struct nsw_routing_bucket;
	struct nsw_settings;
}

namespace libtorrent { namespace nsw
{

struct traversal_algorithm;
struct nsw_logger_observer_interface;
struct msg;

TORRENT_EXTRA_EXPORT entry write_nodes_entry(std::vector<node_entry> const& nodes);

class announce_observer : public observer_interface
{
public:
	announce_observer(std::shared_ptr<traversal_algorithm> const& algo
		, udp::endpoint const& ep, node_id const& id)
		: observer_interface(algo, ep, id)
	{}

	void reply(msg const&) { flags |= flag_done; }
};

struct udp_socket_interface
{
	virtual bool has_quota() = 0;
	virtual bool send_packet(entry& e, udp::endpoint const& addr) = 0;
protected:
	~udp_socket_interface() {}
};

class TORRENT_EXTRA_EXPORT node : boost::noncopyable
{
public:
	node(udp proto, udp_socket_interface* sock
		, libtorrent::nsw_settings const& settings
		, node_id const& nid
		, const std::string& description
		, nsw_logger_observer_interface* observer, counters& cnt
		, std::map<std::string, node*> const& nodes);
//		, nsw_storage_interface& storage);

	~node();

	void update_node_id();

	void tick();
	void bootstrap(std::vector<udp::endpoint> const& nodes);
	//void bootstrap(std::vector<udp::endpoint> const& nodes
	//	, find_data::nodes_callback const& f);
	void add_router_node(udp::endpoint const& router);

	void unreachable(udp::endpoint const& ep);
	void incoming(msg const& m);

#ifndef TORRENT_NO_DEPRECATE
//	int num_torrents() const { return int(m_storage.num_torrents()); }
//	int num_peers() const { return int(m_storage.num_peers()); }
#endif

	int bucket_size(int bucket);

	node_id const& nid() const { return m_id; }

#ifndef TORRENT_DISABLE_LOGGING
	std::uint32_t search_id() { return m_search_id++; }
#endif

	//std::tuple<int, int, int> size() const { return m_table.size(); }



	// std::int64_t num_global_nodes() const
	// { return m_table.num_global_nodes(); }



	enum flags_t { flag_seed = 1, flag_implied_port = 2 };
	void get_peers(sha1_hash const& info_hash
		, std::function<void(std::vector<tcp::endpoint> const&)> dcallback
		, std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> ncallback
		, bool noseeds);
	void announce(sha1_hash const& info_hash, int listen_port, int flags
		, std::function<void(std::vector<tcp::endpoint> const&)> f);

	void direct_request(udp::endpoint const& ep, entry& e
		, std::function<void(msg const&)> f);



	bool verify_token(string_view token, sha1_hash const& info_hash
	, udp::endpoint const& addr) const;

	std::string generate_token(udp::endpoint const& addr, sha1_hash const& info_hash);

	time_duration connection_timeout();

	void new_write_key();

	void add_node(udp::endpoint const& node);
	// to doublecheck!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// void replacement_cache(routing_table_t& nodes) const
	// { m_table.replacement_cache(nodes); }

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

	// void status(std::vector<nsw_routing_bucket>& table
	// 	, std::vector<nsw_lookup>& requests);

	std::tuple<int, int, int> get_stats_counters() const;

	libtorrent::nsw_settings const& settings() const { return m_settings; }
	counters& stats_counters() const { return m_counters; }

	nsw_logger_observer_interface* observer() const { return m_observer; }

	udp protocol() const { return m_protocol.protocol; }
	char const* protocol_family_name() const { return m_protocol.family_name; }
	char const* protocol_nodes_key() const { return m_protocol.nodes_key; }

	bool native_address(udp::endpoint const& ep) const
	{ return ep.protocol().family() == m_protocol.protocol.family(); }
	bool native_address(tcp::endpoint const& ep) const
	{ return ep.protocol().family() == m_protocol.protocol.family(); }
	bool native_address(address const& addr) const
	{
		return (addr.is_v4() && m_protocol.protocol == m_protocol.protocol.v4())
			|| (addr.is_v6() && m_protocol.protocol == m_protocol.protocol.v6());
	}

private:

	void send_single_refresh(udp::endpoint const& ep, int bucket
		, node_id const& id = node_id());
	bool lookup_peers(sha1_hash const& info_hash, entry& reply
		, bool noseed, bool scrape, address const& requester) const;

	libtorrent::nsw_settings const& m_settings;

	std::mutex m_mutex;

	std::set<traversal_algorithm*> m_running_requests;

	void incoming_request(msg const& h, entry& e);

	void write_nodes_entries(sha1_hash const& info_hash
		, bdecode_node const& want, entry& r);

	node_id m_id;

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

	std::map<std::string, node*> const& m_nodes;

	nsw_logger_observer_interface* m_observer;

	protocol_descriptor const& m_protocol;

	time_point m_last_tracker_tick;

	time_point m_last_self_refresh;

	int m_secret[2];

	udp_socket_interface* m_sock;
	counters& m_counters;

//	nsw_storage_interface& m_storage;

public:
	boost::unordered::unordered_map<sha1_hash,boost::shared_ptr<nsw::term_vector> > m_torrents_term_vectors;

#ifndef TORRENT_DISABLE_LOGGING
	std::uint32_t m_search_id = 0;
#endif
};
} }

#endif // NSW_NODE_HPP
