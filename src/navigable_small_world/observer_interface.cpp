#include <libtorrent/navigable_small_world/nsw_observer.hpp>
#include <libtorrent/navigable_small_world/observer_interface.hpp>
#include <libtorrent/navigable_small_world/traversal_algorithm.hpp>
#include <libtorrent/navigable_small_world/node.hpp>

namespace libtorrent { namespace nsw {

inline nsw_observer* observer_interface::get_observer() const
{
    return m_algorithm->get_node().observer();
}

inline void observer_interface::set_target(udp::endpoint const& ep)
{
    m_sent = clock_type::now();

    m_port = ep.port();
#if TORRENT_USE_IPV6
    if (ep.address().is_v6())
    {
        flags |= flag_ipv6_address;
        m_addr.v6 = ep.address().to_v6().to_bytes();
    }
    else
#endif
    {
        flags &= ~flag_ipv6_address;
        m_addr.v4 = ep.address().to_v4().to_bytes();
    }
}

inline address observer_interface::target_addr() const
{
#if TORRENT_USE_IPV6
    if (flags & flag_ipv6_address)
        return address_v6(m_addr.v6);
    else
#endif
        return address_v4(m_addr.v4);
}

inline udp::endpoint observer_interface::target_ep() const
{
    return udp::endpoint(target_addr(), m_port);
}

inline void observer_interface::abort()
{
    if (flags & flag_done) return;
    flags |= flag_done;
    m_algorithm->failed(self(), traversal_algorithm::prevent_request);
}

inline void observer_interface::done()
{
    if (flags & flag_done) return;
    flags |= flag_done;
    m_algorithm->finished(self());
}

inline void observer_interface::short_timeout()
{
    if (flags & flag_short_timeout) return;
    m_algorithm->failed(self(), traversal_algorithm::short_timeout);
}

inline void observer_interface::timeout()
{
    if (flags & flag_done) return;
    flags |= flag_done;
    m_algorithm->failed(self());
}

inline void observer_interface::set_id(node_id const& id)
{
    if (m_id == id) return;
    m_id = id;
    if (m_algorithm) m_algorithm->resort_results();
}

observer_interface::~observer_interface()
{
    // if the message was sent, it must have been
    // reported back to the traversal_algorithm as
    // well. If it wasn't sent, it cannot have been
    // reported back
    TORRENT_ASSERT(m_was_sent == ((flags & flag_done) != 0) || m_was_abandoned);
    TORRENT_ASSERT(!m_in_constructor);
#if TORRENT_USE_ASSERTS
    TORRENT_ASSERT(m_in_use);
    m_in_use = false;
#endif
}

}}
