#ifndef NSW_TRAVERSAL_ALGORITHM_HPP
#define NSW_TRAVERSAL_ALGORITHM_HPP

#include <vector>
#include <map>
//#include <set>


#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/navigable_small_world/observer_interface.hpp"
#include "libtorrent/address.hpp"

#include "libtorrent/aux_/disable_warnings_push.hpp"
#include <boost/noncopyable.hpp>
#include "libtorrent/aux_/disable_warnings_pop.hpp"
namespace libtorrent { struct nsw_lookup; }

namespace libtorrent {
namespace nsw
{
class node;

struct TORRENT_EXTRA_EXPORT traversal_algorithm : boost::noncopyable
	, std::enable_shared_from_this<traversal_algorithm>
{
private:
	node_id const m_nid;
	vector_t const m_target;


	std::int16_t m_invoke_count = 0;
	std::int16_t m_max_threads = 2;
	std::int16_t m_responses = 0;
	std::int16_t m_timeouts = 0;
	std::int16_t m_retrieved_nodes = 0;
#ifndef TORRENT_DISABLE_LOGGING

	std::uint32_t m_id;
#endif

	std::set<std::uint32_t> m_peer4_prefixes;
#if TORRENT_USE_IPV6
	std::set<std::uint64_t> m_peer6_prefixes;
#endif
#ifndef TORRENT_DISABLE_LOGGING
	void log_timeout(observer_ptr const& o, char const* prefix) const;
#endif

protected:
	node& m_node;
	std::vector<observer_ptr> m_results;
public:
	typedef std::vector<std::tuple<node_id, udp::endpoint, std::string, double, uint16_t>> callback_data_t;
	//void traverse(node_id const& id, udp::endpoint const& addr);
	void finished(observer_ptr o);

	enum flags_t { prevent_request = 1, short_timeout = 2 };
	void failed(observer_ptr o, int flags = 0);
	virtual ~traversal_algorithm();
	void status(nsw_lookup& l);

	virtual char const* name() const;
	virtual void start();

	vector_t const& target() const { return m_target; }
	node_id const& target_nid() const { return m_nid; }

	void resort_results();
	void add_entry(node_id const& id
				, vector_t const& descr
				, udp::endpoint const& addr
				, unsigned char flags
				, int lvl
				, int lay);

	traversal_algorithm(node& nsw_node, node_id const& id, vector_t const& target_string);
	int invoke_count() const { TORRENT_ASSERT(m_invoke_count >= 0); return m_invoke_count; }
	int branch_factor() const { TORRENT_ASSERT(m_max_threads >= 0); return m_max_threads; }
	void increase_visited() {++m_retrieved_nodes;};
	node& get_node() const { return m_node; }

#ifndef TORRENT_DISABLE_LOGGING
	std::uint32_t id() const { return m_id; }
#endif

protected:

	std::shared_ptr<traversal_algorithm> self()
	{ return shared_from_this(); }

	bool add_requests();

	void add_gate_entries();
	void init();

	virtual void done();

	virtual observer_ptr new_observer(udp::endpoint const& ep
		, node_id const& id
		, vector_t const& descr
		, int lvl
		, int lay);

	virtual bool invoke(observer_ptr) { return false; }

	int num_responses() const { return m_responses; }
	int num_timeouts() const { return m_timeouts; }
};

class traversal_observer : public observer_interface
{
public:
	traversal_observer(
		std::shared_ptr<traversal_algorithm> const& algorithm
		, udp::endpoint const& ep, node_id const& id, vector_t const& text)
		: observer_interface(algorithm, ep, id, text)
	{}

	virtual void reply(msg const&);
};

} }

#endif // NSW_TRAVERSAL_ALGORITHM_HPP

