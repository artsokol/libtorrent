#ifndef NSW_NODE_ENTRY_HPP
#define NSW_NODE_ENTRY_HPP

#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/socket.hpp"
#include "libtorrent/address.hpp"
#include "libtorrent/union_endpoint.hpp"
#include "libtorrent/time.hpp" // for time_point

#include "libtorrent/navigable_small_world/term_vector.hpp"
#include <string>
#include <unordered_map>

namespace libtorrent { namespace nsw
{

// The node_entry structure represents torrent file on navigable small world map
// we will use this as item in our routing table to keep all necessary
// information about remote nodes.

struct TORRENT_EXTRA_EXPORT node_entry
{
    node_entry(node_id const& id_
            , udp::endpoint const& ep
            , vector_t const& descr
            , int roundtriptime = ~0
            , bool pinged = false);

    explicit node_entry(udp::endpoint const& ep);
    node_entry();

    void update_rtt(int new_rtt);

    bool pinged() const { return timeout_count != ~0; }
    void set_pinged() { if (timeout_count == ~0) timeout_count = 0; }
    void timed_out() const { if (pinged() && timeout_count < 0xfe) ++timeout_count; }
    int fail_count() const { return pinged() ? timeout_count : 0; }
    void reset_fail_count() { if (pinged()) timeout_count = 0; }

    bool confirmed() const { return timeout_count == 0; }

    //Just shortcuts
    address addr() const { return endpoint.address(); }
    int port() const { return endpoint.port; }

    // the time we last received a response for a request to this peer
	time_point last_queried;

	node_id id;

    std::unordered_map<std::string, double> term_vector;

    union_endpoint endpoint;

    // the average RTT of this node
	mutable std::uint16_t rtt;

    // the number of times this node has failed to
    // respond in a row
	mutable std::uint8_t timeout_count;

    #ifndef TORRENT_DISABLE_LOGGING
    time_point first_seen;
    #endif


    bool operator==(node_entry const& a) {
        if (term_vector::getVecSimilarity(a.term_vector,this->term_vector) == 1)
            return true;
        return false;
    }
};

} }

#endif  // NSW_NODE_ENTRY_HPP

