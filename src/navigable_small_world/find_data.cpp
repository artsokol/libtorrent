#include "libtorrent/navigable_small_world/find_data.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/io.hpp"
#include "libtorrent/socket.hpp"
#include "libtorrent/socket_io.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp" // to_hex
#endif

namespace libtorrent { namespace nsw
{

void find_data_observer::reply(msg const& m)
{
	bdecode_node r = m.message.dict_find_dict("r");
	if (!r)
	{
#ifndef TORRENT_DISABLE_LOGGING
		get_observer()->nsw_log(nsw_logger_interface::traversal, "[%u] missing response dict"
			, algorithm()->id());
#endif
		timeout();
		return;
	}

	bdecode_node id = r.dict_find_string("r_id");
	if (!id || id.string_length() != 20)
	{
#ifndef TORRENT_DISABLE_LOGGING
		get_observer()->nsw_log(nsw_logger_interface::traversal, "[%u] invalid id in response"
			, algorithm()->id());
#endif
		timeout();
		return;
	}
	bdecode_node token = r.dict_find_string("token");
	if (token)
	{
		static_cast<find_data*>(algorithm())->got_write_token(
			node_id(id.string_ptr()), token.string_value().to_string());
	}

	traversal_observer::reply(m);
	done();
}

find_data::find_data(
	node& nsw_node
	, node_id const& id
	, vector_t const& target_string
	, nodes_callback const& ncallback)
	: traversal_algorithm(nsw_node, id, target_string)
	, m_nodes_callback(ncallback)
	, m_done(false)
{
}

void find_data::start()
{
	// if the user didn't add seed-nodes manually, grab k
	// nodes from routing table.
	if (m_results.empty())
	{
		std::vector<node_entry> nodes;
		m_node.m_table.find_node(target(), nodes, m_node.m_table.neighbourhood_size(), 0);

		for (auto const& n : nodes)
		{
			add_entry(n.id, n.term_vector, n.endpoint, observer_interface::flag_initial, 0, 1);
		}
	}

	traversal_algorithm::start();
}

void find_data::got_write_token(node_id const& n, std::string write_token)
{
#ifndef TORRENT_DISABLE_LOGGING
	auto logger = get_node().observer();
	if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
	{
		logger->nsw_log(nsw_logger_interface::traversal
			, "[%u] adding write token '%s' under id '%s'"
			, id(), aux::to_hex(write_token).c_str()
			, aux::to_hex(n).c_str());
	}
#endif
	m_write_tokens[n] = std::move(write_token);
}

observer_ptr find_data::new_observer(udp::endpoint const& ep
	, node_id const& id
	, vector_t const& descr
	, int lvl
	, int lay)
{
	auto o = m_node.m_rpc.allocate_observer<find_data_observer>(self(), ep, id, descr);
// #if TORRENT_USE_ASSERTS
// 	if (o) o->m_in_constructor = false;
// #endif
	return o;
}

char const* find_data::name() const { return "find_data"; }

void find_data::done()
{
	m_done = true;

#ifndef TORRENT_DISABLE_LOGGING
	auto logger = get_node().observer();
	if (logger != nullptr)
	{
		logger->nsw_log(nsw_logger_interface::traversal, "[%u] %s DONE"
			, id(), name());
	}
#endif

	std::vector<std::pair<node_entry, std::string>> results;
	int num_results = m_node.m_table.neighbourhood_size();
	for (std::vector<observer_ptr>::iterator i = m_results.begin()
		, end(m_results.end()); i != end && num_results > 0; ++i)
	{
		observer_ptr const& o = *i;
		if ((o->flags & observer_interface::flag_alive) == 0)
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
			{
				logger->nsw_log(nsw_logger_interface::traversal, "[%u] not alive: %s"
					, id(), print_endpoint(o->target_ep()).c_str());
			}
#endif
			continue;
		}
		auto j = m_write_tokens.find(o->id());
		if (j == m_write_tokens.end())
		{
#ifndef TORRENT_DISABLE_LOGGING
			if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
			{
				logger->nsw_log(nsw_logger_interface::traversal, "[%u] no write token: %s"
					, id(), print_endpoint(o->target_ep()).c_str());
			}
#endif
			continue;
		}
		results.push_back(std::make_pair(node_entry(o->id()
												, o->target_ep()
												, o->descr()
												, ~0,false), j->second));
#ifndef TORRENT_DISABLE_LOGGING
		if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
		{
			logger->nsw_log(nsw_logger_interface::traversal, "[%u] %s"
				, id(), print_endpoint(o->target_ep()).c_str());
		}
#endif
		--num_results;
	}

	if (m_nodes_callback) m_nodes_callback(results);

	traversal_algorithm::done();
}

} } // namespace libtorrent::nsw
