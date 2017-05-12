#ifndef NSW_ROUTING_TABLE_HPP
#define NSW_ROUTING_TABLE_HPP

#include <vector>
#include <set>
#include <unordered_set>
#include <cstdint>
#include <tuple>
#include <array>

#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/node_entry.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"
#include "libtorrent/session_settings.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/time.hpp"

// namespace libtorrent
// {
// 	struct nsw_routing_vector;
// }

namespace libtorrent { namespace nsw
{
struct nsw_logger_interface;


using routing_table_t = std::vector<node_entry> ;

typedef struct nsw_routing_info
{
	uint32_t num_nodes;
	uint32_t num_long_links;
} nsw_routing_info;

struct ipv4_hash
{
	using argument_type = address_v4::bytes_type;
	using result_type = std::size_t;
	result_type operator()(argument_type const& ip) const
	{
		// strange expression &ip[0] is just cast error avoiding
		return std::hash<std::uint32_t>()(*reinterpret_cast<std::uint32_t const*>(&ip[0]));
	}
};

#if TORRENT_USE_IPV6
struct ipv6_hash
{
	using argument_type = address_v6::bytes_type;
	using result_type = std::size_t;
	result_type operator()(argument_type const& ip) const
	{
		// strange expression &ip[0] is just cast error avoiding
		return std::hash<std::uint64_t>()(*reinterpret_cast<std::uint64_t const*>(&ip[0]));
	}
};
#endif


namespace impl
{
	template <typename F>
	inline void forwarder(void* userdata, node_entry const& node)
	{
		F* f = reinterpret_cast<F*>(userdata);
		(*f)(node);
	}
}

class TORRENT_EXTRA_EXPORT routing_table
{
private:
	class textComp {
	    vector_t m_text_vec;
	public:
	    textComp(vector_t const descr)
	    :m_text_vec(descr)
	    {}

	    bool operator()(node_entry const& a,
					node_entry const& b) {

	        return term_vector::getVecSimilarity(a.term_vector,m_text_vec) >
					term_vector::getVecSimilarity(b.term_vector,m_text_vec);
	    }
	};

	using replacement_table_t = std::multiset<node_entry
											, textComp>;


	// class gateComp {
	// public:
	//     bool operator()(vector_t const& a,
	// 				vector_t const& b) {

	//         return term_vector::getVecSimilarity(a,b) >
	// 				term_vector::getVecSimilarity(b.term_vector,m_text_vec);
	//     }
	// };
////////////////////////////////////////////////////////

	uint32_t const m_neighbourhood_size = 6;
	// not used. For future restrictions
	uint32_t const m_equal_text_max_nodes;
	nsw_settings const& m_settings;

	routing_table_t m_routing;

	node_id m_id;
	vector_t m_description_vec;
	udp m_protocol;


	mutable time_point m_last_self_refresh;

	// this is a set of all the endpoints that have
	// been identified as router nodes. They will
	// be used in searches as gates, but they should not
	// be added to the routing table.
	std::vector<std::pair<vector_t, udp::endpoint>> m_gate_nodes;

	// stores short links
	routing_table_t m_close_nodes_rt;
	// and long links - friends who was closest some
	// time ago but now are not. We should keep links
	// to provide log() search complexity.
	routing_table_t m_far_nodes_rt;

	// nodes with term_vectors exact the same as in other
	// tables. Need for quick replacements.
	replacement_table_t m_replacement_nodes;
#ifndef TORRENT_DISABLE_LOGGING
	nsw_logger_interface* m_log;
#endif


public:
	enum add_node_status_t {
		failed_to_add = 0,
		node_exists,
		node_added
	};
	enum table_type_t {
		none = -1,
		closest_nodes = 0,
		far_nodes
	};

	routing_table(node_id const& id, udp proto
		, int neighbourhood_size
		, nsw_settings const& settings
		, std::string const& node_description
		, nsw_logger_interface* log);

	routing_table(routing_table const&) = delete;
	routing_table& operator=(routing_table const&) = delete;

	void status(nsw_routing_info& s) const;

	void node_failed(node_id const& nid, udp::endpoint const& ep);

	void add_gate_node(udp::endpoint const& router, vector_t const&);

	// iterates over the router nodes added
	typedef std::vector<std::pair<vector_t, udp::endpoint>>::const_iterator router_iterator;
	router_iterator begin() const { return m_gate_nodes.begin(); }
	router_iterator end() const { return m_gate_nodes.end(); }


	add_node_status_t add_friend_impl(node_entry e);

	bool add_friend(node_entry const& e);

	bool node_seen(node_id const& id
					, vector_t const& vector
					, udp::endpoint const& ep
					, int rtt);

	//update time
	void heard_about(node_id const& id, udp::endpoint const& ep);

	// change our node ID. This can be expensive since nodes must be moved around
	// and potentially dropped
	void update_node_id(node_id const& id);

	void update_description();
	node_entry const* next_for_refresh();


	void remove_node(node_entry* n
		, routing_table_t& rt) ;

	template <typename F>
	void for_each_node(F f)
	{
		for_each_node(&impl::forwarder<F>, &impl::forwarder<F>, reinterpret_cast<void*>(&f));
	}

	void for_each_node(void (*)(void*, node_entry const&)
		, void (*)(void*, node_entry const&), void* userdata) const;

	std::tuple<size_t, size_t, size_t> size() const;

	bool is_full() const;

	bool is_native_address(address const& addr) const
	{
		return (addr.is_v4() && m_protocol == udp::v4())
			|| (addr.is_v6() && m_protocol == udp::v6());
	}

	bool is_native_endpoint(udp::endpoint const& ep) const
	{ return ep.protocol() == m_protocol; }

	node_id const& id() const
	{ return m_id; }

	vector_t const& get_descr() const
	{ return m_description_vec; }

	routing_table_t const& neighbourhood() const
	{ return m_close_nodes_rt; }

	routing_table_t const& old_relations() const
	{ return m_far_nodes_rt; }
	int neighbourhood_size() const { return m_neighbourhood_size; }

	// fills the vector with the k nodes from our storage that
	// are nearest to the given text.
	void find_node(vector_t const& target_string
					, std::vector<node_entry>& l, int count=0);
//	std::int64_t num_global_nodes() const;

//	int num_active_buckets() const { return int(m_buckets.size()); }

//	void replacement_cache(bucket_t& nodes) const;

//	int bucket_limit(int bucket) const;
	// enum
	// {
	// 	// nodes that have not been pinged are considered failed by this flag
	// 	include_failed = 1
	// };

	// fills the vector with the count nodes that
	// are nearest to the given term vector.
	// void find_node(std::string const& id
	// 			, routing_table::table_type_t type
	// 			, int index);

private:

#ifndef TORRENT_DISABLE_LOGGING
	void log_node_failed(node_id const& nid, node_entry const& ne) const;
	void log_node_added(node_entry const& ne) const;
#endif

//	table_t::iterator find_bucket(node_id const& id);
//	void remove_node_internal(node_entry* n, bucket_t& b);

//	void split_bucket();

	node_entry* find_node(udp::endpoint const& ep
							, routing_table::table_type_t& type
							, int& index);


	bool fill_from_replacements(node_entry& ne);

	routing_table::add_node_status_t insert_node(const node_entry& e);
};

} } // namespace libtorrent::nsw

#endif // NSW_ROUTING_TABLE_HPP
