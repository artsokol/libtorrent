#ifndef NSW_RPC_MANAGER_HPP
#define NSW_RPC_MANAGER_HPP

#include <unordered_map>
#include <cstdint>

#include "libtorrent/aux_/disable_warnings_push.hpp"
#include <boost/pool/pool.hpp>
#include "libtorrent/aux_/disable_warnings_pop.hpp"

#include <libtorrent/socket.hpp>
#include <libtorrent/time.hpp>
#include <libtorrent/navigable_small_world/node_id.hpp>
#include <libtorrent/navigable_small_world/observer.hpp>

namespace libtorrent { struct nsw_settings; class entry; }
namespace libtorrent { namespace nsw
{

struct nsw_logger;
struct udp_socket_interface;

struct TORRENT_EXTRA_EXPORT null_observer : public observer
{
	null_observer(std::shared_ptr<traversal_algorithm> const& a
		, udp::endpoint const& ep, node_id const& id): observer(a, ep, id) {}
	virtual void reply(msg const&) { flags |= flag_done; }
};

class routing_table;

class TORRENT_EXTRA_EXPORT rpc_manager
{
public:

	rpc_manager(node_id const& our_id
		, nsw_settings const& settings
		, routing_table& table
		, udp_socket_interface* sock
		, nsw_logger* log);
	~rpc_manager();

	void unreachable(udp::endpoint const& ep);

	bool incoming(msg const&, node_id* id);
	time_duration tick();

	bool invoke(entry& e, udp::endpoint target
		, observer_ptr o);

	void add_our_id(entry& e);

	template <typename T, typename... Args>
	std::shared_ptr<T> allocate_observer(Args&&... args)
	{
		void* ptr = allocate_observer();
		if (ptr == nullptr) return std::shared_ptr<T>();

		auto deleter = [this](observer* o)
		{
			o->~observer();
			free_observer(o);
		};
		return std::shared_ptr<T>(new (ptr) T(std::forward<Args>(args)...), deleter);
	}

	int num_allocated_observers() const { return m_allocated_observers; }

private:

	void* allocate_observer();
	void free_observer(void* ptr);

	std::unordered_multimap<int, observer_ptr> m_transactions;

	udp_socket_interface* m_sock;
#ifndef TORRENT_DISABLE_LOGGING
	nsw_logger* m_log;
#endif
	nsw_settings const& m_settings;
	routing_table& m_table;
	node_id m_our_id;
	std::uint32_t m_allocated_observers:31;
	std::uint32_t m_destructing:1;
};

} }

#endif  // NSW_RPC_MANAGER_HPP


