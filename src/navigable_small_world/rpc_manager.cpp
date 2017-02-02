#include <libtorrent/config.hpp>
#include <libtorrent/io.hpp>
#include <libtorrent/random.hpp>
#include <libtorrent/invariant_check.hpp>

#include <libtorrent/navigable_small_world/rpc_manager.hpp>
#include <libtorrent/navigable_small_world/nsw_routing_table.hpp>
#include <libtorrent/navigable_small_world/node.hpp>
#include <libtorrent/socket_io.hpp>
#include <libtorrent/hasher.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/aux_/time.hpp>

#include <type_traits>
#include <functional>

#ifndef TORRENT_DISABLE_LOGGING
#include <cinttypes>
#endif

using namespace std::placeholders;

namespace libtorrent { namespace nsw {

rpc_manager::rpc_manager(node_id const& our_id
	, nsw_settings const& settings
	, routing_table& table, udp_socket_interface* sock
	, nsw_logger* log)
	: m_sock(sock)
#ifndef TORRENT_DISABLE_LOGGING
	, m_log(log)
#endif
	, m_settings(settings)
	, m_table(table)
	, m_our_id(our_id)
	, m_allocated_observers(0)
	, m_destructing(false)
{
#ifdef TORRENT_DISABLE_LOGGING
	TORRENT_UNUSED(log);
#endif
}

rpc_manager::~rpc_manager()
{
}

void* rpc_manager::allocate_observer()
{
}

void rpc_manager::free_observer(void* ptr)
{
	(void*)ptr;
}


void rpc_manager::unreachable(udp::endpoint const& ep)
{
	(void)ep;
}

bool rpc_manager::incoming(msg const& m, node_id* id)
{
	(void)m;
	(void*)id;
	return false;
}

time_duration rpc_manager::tick()
{
	return duration_cast<time_duration>(milliseconds(200));
}

void rpc_manager::add_our_id(entry& e)
{
	(void)e;
}

bool rpc_manager::invoke(entry& e, udp::endpoint target_addr
	, observer_ptr o)
{
	(void)e;
	(void)target_addr;
	(void)o;
	return false;
}

} }

