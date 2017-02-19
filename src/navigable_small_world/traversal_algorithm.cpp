#include "libtorrent/navigable_small_world/traversal_algorithm.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/socket_io.hpp" // for read_*_endpoint
#include "libtorrent/alert_types.hpp" // for nsw_lookup
#include "libtorrent/aux_/time.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif
using namespace std::placeholders;

namespace libtorrent { namespace nsw
{

#if TORRENT_USE_ASSERTS
template <class It, class Cmp>
bool is_sorted(It b, It e, Cmp cmp)
{
	return true;
}
#endif

observer_ptr traversal_algorithm::new_observer(udp::endpoint const& ep
	, node_id const& id)
{
	observer_ptr o;
	return o;
}

traversal_algorithm::traversal_algorithm(
	node& nsw_node
	, node_id const& target)
	: m_node(nsw_node)
	, m_target(target)
{

}

void traversal_algorithm::resort_results()
{
}

void traversal_algorithm::add_entry(node_id const& id
	, udp::endpoint const& addr, unsigned char const flags)
{

}

void traversal_algorithm::start()
{
}

char const* traversal_algorithm::name() const
{
	return "traversal_algorithm";
}

void traversal_algorithm::traverse(node_id const& id, udp::endpoint const& addr)
{

}

void traversal_algorithm::finished(observer_ptr o)
{
}


void traversal_algorithm::failed(observer_ptr o, int const flags)
{

}

#ifndef TORRENT_DISABLE_LOGGING
void traversal_algorithm::log_timeout(observer_ptr const& o, char const* prefix) const
{

}
#endif

void traversal_algorithm::done()
{

}

bool traversal_algorithm::add_requests()
{

	return false;
}

void traversal_algorithm::add_router_entries()
{
}

void traversal_algorithm::init()
{

}

traversal_algorithm::~traversal_algorithm()
{
}

void traversal_algorithm::status(nsw_lookup& l)
{
}

void traversal_observer::reply(msg const& m)
{
}

} } // namespace libtorrent::nsw
