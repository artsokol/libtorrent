#include "libtorrent/navigable_small_world/bootstrap.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/observer_interface.hpp"
#include "libtorrent/performance_counters.hpp"

#include "libtorrent/io.hpp"


namespace libtorrent { namespace nsw
{

observer_ptr bootstrap::new_observer(udp::endpoint const& ep
	, node_id const& id, vector_t const& text)
{
	auto o = m_node.m_rpc.allocate_observer<get_friends_observer>(self(), ep, id, text, m_exact_search);

	double similarity = term_vector::getVecSimilarity(o->descr(),target());
	auto inserted_item = candidates.emplace(o->id(),o->target_ep(),"","",similarity,get_friends::row::visiting,0);
// #if TORRENT_USE_ASSERTS
// 	if (o) o->m_in_constructor = false;
// #endif
	return o;
}

bool bootstrap::invoke(observer_ptr o)
{
	return get_friends::invoke(o);
}

bootstrap::bootstrap(
	node& nsw_node
	, node_id const& nid
	, vector_t const& target_text
	, done_callback const& callback
	, data_callback const& result_callback
	, bool exact)
	: get_friends(nsw_node, target_text, result_callback, callback)
	, m_exact_search(exact)
{
}

char const* bootstrap::name() const { return "bootstrap"; }

void bootstrap::trim_seed_nodes()
{
	// when we're bootstrapping, we want to start as far away from our ID as
	// possible, to cover as much as possible of the ID space. So, remove all
	// nodes except for the 32 that are farthest away from us
	// if (m_results.size() > 32)
	// 	m_results.erase(m_results.begin(), m_results.end() - 32);
}

void bootstrap::done()
{
#ifndef TORRENT_DISABLE_LOGGING
	get_node().observer()->nsw_log(nsw_logger_interface::traversal, "[%u] bootstrap done, pinging remaining nodes"
		, id());
#endif

	// for (auto const& o : m_results)
	// {
	// 	if (o->flags & observer_interface::flag_queried) continue;
	// 	// this will send a ping
	// 	// call is not correct!! replace id!!!!!!
	// 	m_node.add_node(o->target_ep(),o->id());
	// }
	//get_friends::done();
}

} } // namespace libtorrent::nsw
