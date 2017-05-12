#include "libtorrent/navigable_small_world/node_entry.hpp"
#include "libtorrent/aux_/time.hpp"

namespace libtorrent {
namespace nsw
{

	node_entry::node_entry(node_id const& id_
					, udp::endpoint const& ep
					, std::unordered_map<std::string, double> const& vector
					, int roundtriptime
					, bool pinged
		)
		: last_queried(pinged ? aux::time_now() : min_time())
		, id(id_)
		, term_vector(vector)
		, endpoint(ep)
		, rtt(roundtriptime & ~0)
		, timeout_count(pinged ? 0 : ~0)
#ifndef TORRENT_DISABLE_LOGGING
		, first_seen(aux::time_now())
#endif
	{}

	node_entry::node_entry(udp::endpoint const& ep)
		: last_queried(min_time())
		, id(nullptr)
		, term_vector()
		, endpoint(ep)
		, rtt(~0)
		, timeout_count(~0)
#ifndef TORRENT_DISABLE_LOGGING
		, first_seen(aux::time_now())
#endif
	{}

	node_entry::node_entry()
		: last_queried(min_time())
		, id(nullptr)
		, term_vector()
		, rtt(~0)
		, timeout_count(~0)
#ifndef TORRENT_DISABLE_LOGGING
		, first_seen(aux::time_now())
#endif
	{}


	void node_entry::update_rtt(int new_rtt)
	{
		if (new_rtt == ~0) return;
		if (rtt == ~0) rtt = std::uint16_t(new_rtt);
		else rtt = std::uint16_t(int(rtt) * 2 / 3 + new_rtt / 3);
	}

}}


