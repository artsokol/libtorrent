#include "libtorrent/config.hpp"

#include <utility>
#include <cinttypes>
#include <functional>
#include <tuple>
#include <array>

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif

#include <libtorrent/socket_io.hpp>
#include <libtorrent/session_status.hpp>
#include "libtorrent/bencode.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/random.hpp"
#include <libtorrent/assert.hpp>
#include <libtorrent/aux_/time.hpp>
#include "libtorrent/alert_types.hpp"
#include "libtorrent/performance_counters.hpp"

#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"

namespace libtorrent { namespace nsw {

namespace {

void nop() {}

node_id calculate_node_id(node_id const& nid, nsw_logger_observer_interface* observer, udp protocol)
{
	(void)nid;
	(void*)observer;
	(void)protocol;
	return nid;
}

void incoming_error(entry& e, char const* msg, int error_code = 203)
{
	(void)e;
	(void*)msg;
	(void)error_code;
}
} // anonymous namespace


node::node(udp proto, udp_socket_interface* sock
	, nsw_settings const& settings
	, node_id const& nid
	, nsw_logger_observer_interface* observer
	, counters& cnt
	, std::map<std::string, node*> const& nodes
	, nsw_storage_interface& storage)
	: m_settings(settings)
	, m_id(calculate_node_id(nid, observer, proto))
	, m_table(m_id, proto, 8, settings, observer)
	, m_rpc(m_id, m_settings, m_table, sock, observer)
	, m_nodes(nodes)
	, m_observer(observer)
	, m_protocol(map_protocol_to_descriptor(proto))
	, m_last_tracker_tick(aux::time_now())
	, m_last_self_refresh(min_time())
	, m_sock(sock)
	, m_counters(cnt)
	, m_storage(storage)
{

}

node::~node() = default;

void node::update_node_id()
{
}

bool node::verify_token(string_view token, sha1_hash const& info_hash
	, udp::endpoint const& addr) const
{
	(void)token;
	(void)info_hash;
	(void)addr;

	return false;
}

std::string node::generate_token(udp::endpoint const& addr, sha1_hash const& info_hash)
{
	std::string token;
	(void)info_hash;
	(void)addr;
	return token;
}

void node::bootstrap(std::vector<udp::endpoint> const& nodes)
{
	(void)nodes;

}

int node::bucket_size(int bucket)
{
	(void)bucket;
	return 0;
}

void node::new_write_key()
{

}

void node::unreachable(udp::endpoint const& ep)
{
	(void)ep;
}

void node::incoming(msg const& m)
{
	(void)m;
}

namespace
{
	void announce_fun(std::vector<std::pair<node_entry, std::string> > const& v
		, node& node, int listen_port, sha1_hash const& ih, int flags)
	{
		(void)v;
		(void)node;
		(void)listen_port;
		(void)ih;
		(void)flags;
	}
}

void node::add_router_node(udp::endpoint const& router)
{

}

void node::add_node(udp::endpoint const& node)
{

}

void node::get_peers(sha1_hash const& info_hash
		, std::function<void(std::vector<tcp::endpoint> const&)> dcallback
		, std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> ncallback
		, bool noseeds)
{

}

void node::announce(sha1_hash const& info_hash, int listen_port, int flags
		, std::function<void(std::vector<tcp::endpoint> const&)> f)
{

}

void direct_request(udp::endpoint const& ep, entry& e
		, std::function<void(msg const&)> f)
{

}

void get_item(sha1_hash const& target, std::function<void(item const&)> f)
{

}

void get_item(public_key const& pk, std::string const& salt, std::function<void(item const&, bool)> f)
{
}

namespace {

/*void put(std::vector<std::pair<node_entry, std::string>> const& nodes
	, std::shared_ptr<put_data> ta)
{

}

void put_data_cb(item i, bool auth
	, std::shared_ptr<put_data> ta
	, std::function<void(item&)> f)
{
}*/

} // namespace


void put_item(sha1_hash const& target, entry const& data, std::function<void(int)> f)
{

}

void put_item(public_key const& pk, std::string const& salt
		, std::function<void(item const&, int)> f
		, std::function<void(item&)> data_cb)
{

}

struct ping_observer : observer_interface
{
	ping_observer(
		std::shared_ptr<traversal_algorithm> const& algorithm
		, udp::endpoint const& ep, node_id const& id)
		: observer_interface(algorithm, ep, id)
	{}

	virtual void reply(msg const& m)
	{
		(void)m;
	}
};

void node::tick()
{

}

void node::send_single_refresh(udp::endpoint const& ep, int bucket
	, node_id const& id)
{

}

time_duration node::connection_timeout()
{
	time_duration d;

	return d;
}

void node::status(std::vector<nsw_routing_bucket>& table
	, std::vector<nsw_lookup>& requests)
{

}

#ifndef TORRENT_NO_DEPRECATE
void node::status(session_status& s)
{
	(void)s;
}
#endif

bool node::lookup_peers(sha1_hash const& info_hash, entry& reply
	, bool noseed, bool scrape, address const& requester) const
{
	(void)info_hash;
	(void)reply;
	(void)noseed;
	(void)scrape;
	(void)requester;

	return false;
}

entry write_nodes_entry(std::vector<node_entry> const& nodes)
{
	entry r;
	(void)nodes;
	return r;
}

void node::incoming_request(msg const& m, entry& e)
{
	(void)m;
	(void)e;
}

void node::write_nodes_entries(sha1_hash const& info_hash
	, bdecode_node const& want, entry& r)
{
	(void)info_hash;
	(void)want;
	(void)r;
}

node::protocol_descriptor const& node::map_protocol_to_descriptor(udp protocol)
{
	static std::array<protocol_descriptor, 2> descriptors =
	{{
		{udp::v4(), "n4", "nodes"},
		{udp::v6(), "n6", "nodes6"}
	}};

	for (auto const& d : descriptors)
	{
		if (d.protocol == protocol)
			return d;
	}

	TORRENT_ASSERT_FAIL();
#ifndef BOOST_NO_EXCEPTIONS
	throw std::out_of_range("unknown protocol");
#else
	std::terminate();
#endif
}


} }
