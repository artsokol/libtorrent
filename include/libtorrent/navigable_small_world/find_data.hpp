#ifndef FIND_DATA_HPP
#define FIND_DATA_HPP

#include "libtorrent/navigable_small_world/traversal_algorithm.hpp"
#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/nsw_routing_table.hpp"
#include "libtorrent/navigable_small_world/rpc_manager.hpp"
#include "libtorrent/navigable_small_world/observer_interface.hpp"
#include "libtorrent/navigable_small_world/msg.hpp"

#include <vector>
#include <map>

namespace libtorrent { namespace nsw
{

class node;

// -------- find data -----------

struct find_data : traversal_algorithm
{
public:
	typedef std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> nodes_callback;

protected:
	nodes_callback m_nodes_callback;
	std::map<node_id, std::string> m_write_tokens;
	bool m_done;

public:
	find_data(node& nsw_node
			, node_id const& nid
			, vector_t const& target_string
			, nodes_callback const& ncallback);

	void got_write_token(node_id const& n, std::string write_token);

	virtual void start();

	virtual char const* name() const;

//	virtual ~find_data();
protected:

	virtual void done();
	virtual observer_ptr new_observer(udp::endpoint const& ep
		, node_id const& id
		, vector_t const& text
		, int lvl
		, int lay);


};

class find_data_observer : public traversal_observer
{
public:
	find_data_observer(
		std::shared_ptr<traversal_algorithm> const& algorithm
		, udp::endpoint const& ep, node_id const& id, vector_t const& text)
		: traversal_observer(algorithm, ep, id, text)
	{}

	virtual void reply(msg const&);
};

} } // namespace libtorrent::nsw

#endif // FIND_DATA_HPP
