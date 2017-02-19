#include "libtorrent/navigable_small_world/nsw_tracker.hpp"

#include "libtorrent/config.hpp"

#include "libtorrent/navigable_small_world/msg.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"

#include "libtorrent/bencode.hpp"
#include "libtorrent/version.hpp"
#include "libtorrent/time.hpp"
#include "libtorrent/performance_counters.hpp"
#include "libtorrent/aux_/time.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/session_settings.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif

using namespace std::placeholders;

namespace libtorrent { namespace nsw
{
namespace {

	time_duration const key_refresh
		= duration_cast<time_duration>(minutes(5));

	void add_nsw_counters(node const& nsw, counters& c)
	{
		int nodes, replacements, allocated_observers;
		std::tie(nodes, replacements, allocated_observers) = nsw.get_stats_counters();

		c.inc_stats_counter(counters::nsw_nodes, nodes);
		//c.inc_stats_counter(counters::nsw_node_cache, replacements);
		//c.inc_stats_counter(counters::nsw_allocated_observers, allocated_observers);
	}

	} // anonymous namespace


	nsw_tracker::nsw_tracker(nsw_logger_observer_interface* observer
		, io_service& ios
		, send_fun_t const& send_fun
		, nsw_settings const& settings
		, counters& cnt
//		, nsw_storage_interface& storage
		, nsw_state state)
		: m_counters(cnt)
//		, m_storage(storage)
		, m_state(std::move(state))
		, m_nsw(udp::v4(), this, settings, m_state.nid
			, observer, cnt, m_nodes)
#if TORRENT_USE_IPV6
		, m_nsw6(udp::v6(), this, settings, m_state.nid6
			, observer, cnt, m_nodes)
#endif
		, m_send_fun(send_fun)
		, m_log(observer)
		, m_key_refresh_timer(ios)
		, m_connection_timer(ios)
#if TORRENT_USE_IPV6
		, m_connection_timer6(ios)
#endif
		, m_refresh_timer(ios)
		, m_settings(settings)
		, m_abort(false)
		, m_host_resolver(ios)
		, m_last_tick(aux::time_now())
	{

	}

	nsw_tracker::~nsw_tracker() = default;

	void nsw_tracker::update_node_id()
	{

	}

	void nsw_tracker::start()
	{
	}

	void nsw_tracker::stop()
	{
	}

#ifndef TORRENT_NO_DEPRECATE
	void nsw_tracker::nsw_status(session_status& s)
	{
		(void)s;
	}
#endif

	void nsw_tracker::nsw_status(std::vector<nsw_routing_bucket>& table
		, std::vector<nsw_lookup>& requests)
	{
		(void)table;
		(void)requests;
	}

	void nsw_tracker::update_stats_counters(counters& c) const
	{
		(void)c;
	}

	void nsw_tracker::connection_timeout(node& n, error_code const& e)
	{
		(void)n;
		(void)e;
	}

	void nsw_tracker::refresh_timeout(error_code const& e)
	{
		(void)e;
	}

	void nsw_tracker::refresh_key(error_code const& e)
	{
		(void)e;
	}

	void nsw_tracker::update_storage_node_ids()
	{
	}

	// void nsw_tracker::get_peers(sha1_hash const& ih
	// 	, std::function<void(std::vector<tcp::endpoint> const&)> f)
	// {
	// 	(void)ih;
	// 	(void)f;
	// }

	void nsw_tracker::announce(sha1_hash const& ih, int listen_port, int flags
		, std::function<void(std::vector<tcp::endpoint> const&)> f)
	{
		(void)ih;
		(void)listen_port;
		(void)flags;
		(void)f;
	}

	namespace {

	// struct get_immutable_item_ctx
	// {
	// 	explicit get_immutable_item_ctx(int traversals)
	// 		: active_traversals(traversals)
	// 		, item_posted(false)
	// 	{}
	// 	int active_traversals;
	// 	bool item_posted;
	// };

	// void get_immutable_item_callback(item const& it, std::shared_ptr<get_immutable_item_ctx> ctx
	// 	, std::function<void(item const&)> f)
	// {
	// 	(void)it;
	// 	(void)ctx;
	// 	(void)f;
	// }

	// struct get_mutable_item_ctx
	// {
	// 	explicit get_mutable_item_ctx(int traversals) : active_traversals(traversals) {}
	// 	int active_traversals;
	// 	item it;
	// };

	// void get_mutable_item_callback(item const& it, bool authoritative
	// 	, std::shared_ptr<get_mutable_item_ctx> ctx
	// 	, std::function<void(item const&, bool)> f)
	// {
	// 	(void)it;
	// 	(void)authoritative;
	// 	(void)ctx;
	// 	(void)f;
	// }

	// struct put_item_ctx
	// {
	// 	explicit put_item_ctx(int traversals)
	// 		: active_traversals(traversals)
	// 		, response_count(0)
	// 	{}

	// 	int active_traversals;
	// 	int response_count;
	// };

	// void put_immutable_item_callback(int responses, std::shared_ptr<put_item_ctx> ctx
	// 	, std::function<void(int)> f)
	// {
	// 	(void) responses;
	// 	(void)ctx;
	// 	(void)f;
	// }

	// void put_mutable_item_callback(item const& it, int responses, std::shared_ptr<put_item_ctx> ctx
	// 	, std::function<void(item const&, int)> cb)
	// {
	// 	(void)it;
	// 	(void)responses;
	// 	(void)ctx;
	// 	(void)cb;
	// }

	} // anonymous namespace

	// void nsw_tracker::get_item(sha1_hash const& target
	// 	, std::function<void(item const&)> cb)
	// {
	// 	(void)target;
	// 	(void)cb;
	// }

	// key is a 32-byte binary string, the public key to look up.
	// the salt is optional
	// void nsw_tracker::get_item(public_key const& key
	// 	, std::function<void(item const&, bool)> cb
	// 	, std::string salt)
	// {
	// 	(void)key;
	// 	(void)cb;
	// 	(void)salt;
	// }

	// void nsw_tracker::put_item(entry const& data
	// 	, std::function<void(int)> cb)
	// {
	// 	(void)data;
	// 	(void)cb;
	// }

	// void nsw_tracker::put_item(public_key const& key
	// 	, std::function<void(item const&, int)> cb
	// 	, std::function<void(item&)> data_cb, std::string salt)
	// {
	// 	(void)key;
	// 	(void)cb;
	// 	(void)data_cb;
	// 	(void)salt;
	// }

	void nsw_tracker::direct_request(udp::endpoint const& ep, entry& e
		, std::function<void(msg const&)> f)
	{
		(void)ep;
		(void)e;
		(void)f;
	}

	void nsw_tracker::incoming_error(error_code const& ec, udp::endpoint const& ep)
	{
		(void)ec;
		(void)ep;
	}

	bool nsw_tracker::incoming_packet(udp::endpoint const& ep
		, span<char const> const buf)
	{
		(void)ep;
		(void)buf;
		return false;
	}

	namespace {

	void add_node_fun(void* userdata, node_entry const& e)
	{
		(void*)userdata;
		(void)e;
	}

	std::vector<udp::endpoint> save_nodes(node const& nsw)
	{
		std::vector<udp::endpoint> ret;
		(void)nsw;
		return ret;
	}

} // anonymous namespace


	void nsw_tracker::add_node(udp::endpoint const& node)
	{
		(void)node;
	}

	void nsw_tracker::add_router_node(udp::endpoint const& node)
	{
		(void)node;
	}

	bool nsw_tracker::has_quota()
	{
		return false;
	}

	bool nsw_tracker::send_packet(entry& e, udp::endpoint const& addr)
	{
		(void)e;
		(void)addr;
		return false;
	}

}}

