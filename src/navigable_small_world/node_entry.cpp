#include "libtorrent/navigable_small_world/node_entry.hpp"
#include "libtorrent/aux_/time.hpp"

namespace libtorrent {
namespace nsw
{

	node_entry::node_entry(node_id const& id_, udp::endpoint ep
		, int roundtriptime
		, bool pinged)
		: last_queried(pinged ? aux::time_now() : min_time())
		, id(id_)
//		, term()
		, a(ep.address().to_v4().to_bytes())
		, p(ep.port())
		, rtt(roundtriptime & 0xffff)
		, timeout_count(pinged ? 0 : 0xff)
	{

	}

	node_entry::node_entry(udp::endpoint ep)
		: last_queried(min_time())
		, id(0)
//	    , term()
		, a(ep.address().to_v4().to_bytes())
		, p(ep.port())
		, rtt(0xffff)
		, timeout_count(0xff)
	{

	}

	node_entry::node_entry()
		: last_queried(min_time())
		, id(0)
//		, term()
		, p(0)
		, rtt(0xffff)
		, timeout_count(0xff)
	{

	}

	void node_entry::update_rtt(int new_rtt)
	{
		(void)new_rtt;
	}

}}


