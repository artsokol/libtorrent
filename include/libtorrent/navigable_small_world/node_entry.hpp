#ifndef NSW_NODE_ENTRY_HPP
#define NSW_NODE_ENTRY_HPP

#include "libtorrent/navigable_small_world/node_id.hpp"
//#include "libtorrent/navigable_small_world/term_vector.hpp"
#include "libtorrent/socket.hpp"
#include "libtorrent/address.hpp"
#include "libtorrent/union_endpoint.hpp"
#include "libtorrent/time.hpp" // for time_point

namespace libtorrent { namespace nsw
{

struct TORRENT_EXTRA_EXPORT node_entry
{
	node_entry(node_id const& id_, udp::endpoint ep, int roundtriptime = 0xffff
		, bool pinged = false);
	node_entry(udp::endpoint ep);
	node_entry();
	void update_rtt(int new_rtt);

	bool pinged() const { return timeout_count != 0xff; }
	void set_pinged() { if (timeout_count == 0xff) timeout_count = 0; }
	void timed_out() { if (pinged() && timeout_count < 0xfe) ++timeout_count; }
	int fail_count() const { return pinged() ? timeout_count : 0; }
	void reset_fail_count() { if (pinged()) timeout_count = 0; }
	udp::endpoint ep() const { return udp::endpoint(address_v4(a), p); }
	bool confirmed() const { return timeout_count == 0; }
	address addr() const { return address_v4(a); }
	int port() const { return p; }

	time_point last_queried;

	node_id id;

    //nsw::term_vector term;

	address_v4::bytes_type a;
	boost::uint16_t p;

	boost::uint16_t rtt;

	boost::uint8_t timeout_count;
};

} }

#endif  // NSW_NODE_ENTRY_HPP

