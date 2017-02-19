
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
#include "libtorrent/session_settings.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/time.hpp"

namespace libtorrent
{
#ifndef TORRENT_NO_DEPRECATE
	struct session_status;
#endif
	struct nsw_routing_bucket;
}

namespace libtorrent { namespace nsw
{
struct nsw_logger_interface;

typedef std::vector<node_entry> bucket_t;

struct routing_table_node
{
	bucket_t replacements;
	bucket_t live_nodes;
};

struct ipv4_hash
{
	using argument_type = address_v4::bytes_type;
	using result_type = std::size_t;
	result_type operator()(argument_type const& ip) const
	{
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
		return std::hash<std::uint64_t>()(*reinterpret_cast<std::uint64_t const*>(&ip[0]));
	}
};
#endif

struct ip_set
{
	void insert(address const& addr);
	bool exists(address const& addr) const;
	void erase(address const& addr);

	void clear()
	{
		m_ip4s.clear();
#if TORRENT_USE_IPV6
		m_ip6s.clear();
#endif
	}

	bool operator==(ip_set const& rh)
	{
#if TORRENT_USE_IPV6
		return m_ip4s == rh.m_ip4s && m_ip6s == rh.m_ip6s;
#else
		return m_ip4s == rh.m_ip4s;
#endif
	}


	std::unordered_multiset<address_v4::bytes_type, ipv4_hash> m_ip4s;
#if TORRENT_USE_IPV6
	std::unordered_multiset<address_v6::bytes_type, ipv6_hash> m_ip6s;
#endif
};


namespace impl
{
	template <typename F>
	inline void forwarder(void* userdata, node_entry const& node)
	{
		F* f = reinterpret_cast<F*>(userdata);
		(*f)(node);
	}
}

TORRENT_EXTRA_EXPORT bool compare_ip_cidr(address const& lhs, address const& rhs);

class TORRENT_EXTRA_EXPORT routing_table
{
public:

	using table_t = std::vector<routing_table_node>;

	routing_table(node_id const& id, udp proto
		, int bucket_size
		, nsw_settings const& settings
		, nsw_logger_interface* log);

	routing_table(routing_table const&) = delete;
	routing_table& operator=(routing_table const&) = delete;

#ifndef TORRENT_NO_DEPRECATE
	void status(session_status& s) const;
#endif

	void status(std::vector<nsw_routing_bucket>& s) const;

	void node_failed(node_id const& id, udp::endpoint const& ep);

	void add_router_node(udp::endpoint const& router);


	typedef std::set<udp::endpoint>::const_iterator router_iterator;
	router_iterator begin() const { return m_router_nodes.begin(); }
	router_iterator end() const { return m_router_nodes.end(); }

	enum add_node_status_t {
		failed_to_add = 0,
		node_added,
		need_bucket_split
	};
	add_node_status_t add_node_impl(node_entry e);

	bool add_node(node_entry const& e);

	bool node_seen(node_id const& id, udp::endpoint const& ep, int rtt);

	void heard_about(node_id const& id, udp::endpoint const& ep);

	void update_node_id(node_id const& id);

	node_entry const* next_refresh();

	enum
	{
		include_failed = 1
	};

	void find_node(node_id const& id, std::vector<node_entry>& l
		, int options, int count = 0);
	void remove_node(node_entry* n
		, table_t::iterator bucket) ;

	int bucket_size(int bucket) const
	{
		int num_buckets = int(m_buckets.size());
		if (num_buckets == 0) return 0;
		if (bucket >= num_buckets) bucket = num_buckets - 1;
		table_t::const_iterator i = m_buckets.begin();
		std::advance(i, bucket);
		return int(i->live_nodes.size());
	}

	template <typename F>
	void for_each_node(F f)
	{
		for_each_node(&impl::forwarder<F>, &impl::forwarder<F>, reinterpret_cast<void*>(&f));
	}

	void for_each_node(void (*)(void*, node_entry const&)
		, void (*)(void*, node_entry const&), void* userdata) const;

	int bucket_size() const { return m_bucket_size; }

	std::tuple<int, int, int> size() const;

	std::int64_t num_global_nodes() const;

	int depth() const;

	int num_active_buckets() const { return int(m_buckets.size()); }

	void replacement_cache(bucket_t& nodes) const;

	int bucket_limit(int bucket) const;

#if TORRENT_USE_INVARIANT_CHECKS
	void check_invariant() const;
#endif

	bool is_full(int bucket) const;

	bool native_address(address const& addr) const
	{
		return (addr.is_v4() && m_protocol == udp::v4())
			|| (addr.is_v6() && m_protocol == udp::v6());
	}

	bool native_endpoint(udp::endpoint const& ep) const
	{ return ep.protocol() == m_protocol; }

	node_id const& id() const
	{ return m_id; }

	table_t const& buckets() const
	{ return m_buckets; }

private:

#ifndef TORRENT_DISABLE_LOGGING
	nsw_logger_interface* m_log;
	void log_node_failed(node_id const& nid, node_entry const& ne) const;
#endif

	table_t::iterator find_bucket(node_id const& id);
	void remove_node_internal(node_entry* n, bucket_t& b);

	void split_bucket();

	node_entry* find_node(udp::endpoint const& ep
		, routing_table::table_t::iterator* bucket);

	void fill_from_replacements(table_t::iterator bucket);

	nsw_settings const& m_settings;

	table_t m_buckets;

	node_id m_id;
	udp m_protocol;

	mutable int m_depth;

	mutable time_point m_last_self_refresh;

	std::set<udp::endpoint> m_router_nodes;

	ip_set m_ips;


	int const m_bucket_size;
};

} } // namespace libtorrent::nsw

#endif // NSW_ROUTING_TABLE_HPP
