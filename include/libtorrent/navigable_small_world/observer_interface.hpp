#ifndef MAIN_OBSERVER_INTERFACE_HPP
#define MAIN_OBSERVER_INTERFACE_HPP

#include <cstdint>
#include <memory>

#include <libtorrent/navigable_small_world/node_id.hpp>
#include <libtorrent/navigable_small_world/msg.hpp>
#include <libtorrent/navigable_small_world/nsw_observer.hpp>
#include <libtorrent/time.hpp>
#include <libtorrent/address.hpp>

namespace libtorrent { namespace nsw {

//struct nsw_observer;
//struct msg;
struct traversal_algorithm;
//class observer_interface;

//definition of observer interface (pattern)
class TORRENT_EXTRA_EXPORT observer_interface : boost::noncopyable
	, std::enable_shared_from_this<observer_interface>
{
	std::shared_ptr<observer_interface> self() { return shared_from_this(); }

	time_point m_sent;

	const std::shared_ptr<traversal_algorithm> m_algorithm;

	node_id m_id;

	union addr_t
	{
#if TORRENT_USE_IPV6
		address_v6::bytes_type v6;
#endif
		address_v4::bytes_type v4;
	} m_addr;

	std::uint16_t m_port;

	std::uint16_t m_transaction_id;

protected:
	void done();

public:
	observer_interface(std::shared_ptr<traversal_algorithm> const& a
		, udp::endpoint const& ep, node_id const& id)
		: m_sent()
		, m_algorithm(a)
		, m_id(id)
		, m_port(0)
		, m_transaction_id()
		, flags(0)
	{
		TORRENT_ASSERT(a);
#if TORRENT_USE_ASSERTS
		m_in_constructor = true;
		m_was_sent = false;
		m_was_abandoned = false;
		m_in_use = true;
#endif
		set_target(ep);
	}

	virtual ~observer_interface();

	// this is called when a reply is received
	virtual void reply(msg const& m) = 0;

	// this is called if no response has been received after
	// a few seconds, before the request has timed out
	void short_timeout();

	bool has_short_timeout() const { return (flags & flag_short_timeout) != 0; }

	// this is called when no reply has been received within
	// some timeout, or a reply with incorrect format.
	virtual void timeout();

	void abort();

	nsw_observer* get_observer() const;

	traversal_algorithm* algorithm() const { return m_algorithm.get(); }

	time_point sent() const { return m_sent; }

	void set_target(udp::endpoint const& ep);
	address target_addr() const;
	udp::endpoint target_ep() const;

	void set_id(node_id const& id);
	node_id const& id() const { return m_id; }

	void set_transaction_id(std::uint16_t tid)
	{ m_transaction_id = tid; }

	std::uint16_t transaction_id() const
	{ return m_transaction_id; }

	enum {
		flag_queried = 1,
		flag_initial = 2,
		flag_no_id = 4,
		flag_short_timeout = 8,
		flag_failed = 16,
		flag_ipv6_address = 32,
		flag_alive = 64,
		flag_done = 128
	};


	std::uint8_t flags;

#if TORRENT_USE_ASSERTS
	bool m_in_constructor:1;
	bool m_was_sent:1;
	bool m_was_abandoned:1;
	bool m_in_use:1;
#endif
};

typedef std::shared_ptr<observer_interface> observer_ptr;

} }

#endif // MAIN_OBSERVER_INTERFACE_HPP
