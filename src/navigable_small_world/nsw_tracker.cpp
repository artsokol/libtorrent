#include "libtorrent/navigable_small_world/nsw_tracker.hpp"

#include "libtorrent/config.hpp"

#include "libtorrent/navigable_small_world/msg.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"

#include "libtorrent/bencode.hpp"
#include "libtorrent/version.hpp"
#include "libtorrent/time.hpp"
#include "libtorrent/performance_counters.hpp"
#include "libtorrent/aux_/time.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/session_settings.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif

using namespace std::placeholders;

namespace libtorrent { namespace nsw {
//namespace {

	time_duration const key_refresh
		= duration_cast<time_duration>(minutes(15));
	time_duration const ping_send_timeout
		= duration_cast<time_duration>(seconds(5));
	// void add_nsw_counters(node const& nsw, counters& c)
	// {
	// 	int nodes, replacements, allocated_observers;
	// 	std::tie(nodes, replacements, allocated_observers) = nsw.get_stats_counters();

	// 	c.inc_stats_counter(counters::nsw_nodes, nodes);
	// 	c.inc_stats_counter(counters::nsw_node_cache, replacements);
	// 	c.inc_stats_counter(counters::nsw_allocated_observers, allocated_observers);
	// }

//	} // anonymous namespace


	nsw_tracker::nsw_tracker(nsw_logger_observer_interface* observer
		, io_service& ios
		, send_fun_t const& send_fun
		, nsw_settings const& settings
		, counters& cnt
//		, nsw_storage_interface& storage
		, nsw_state state)
		: m_counters(cnt)
//		, m_storage(storage)
		, m_state(std::move(state))
//		, m_nsw_templ(udp::v4(), this, settings, m_state.nid, ""
//			, observer, cnt)
		, m_send_fun(send_fun)
		, m_log(observer)
		, m_timers_vec()
		// , m_key_refresh_timer(ios)
		// , m_connection_timer(ios)
		// , m_refresh_timer(ios)
		, m_settings(settings)
		, m_status(not_init)
//		, m_host_resolver(ios)
		, m_last_tick(aux::time_now())
		, m_io_srv(ios)
		, m_current_callback(nullptr)
	{
//		m_nodes.insert(std::make_pair(m_nsw_templ.protocol_family_name(), &m_nsw_templ));


//		update_storage_node_ids();

#ifndef TORRENT_DISABLE_LOGGING
		if (m_log->should_log(nsw_logger_interface::tracker))
		{
			m_log->nsw_log(nsw_logger_interface::tracker, "starting IPv4 NSW tracker");
#endif
		}
	}

	nsw_tracker::~nsw_tracker() = default;

	void nsw_tracker::update_node_id()
	{
		for_each(m_nodes.begin(),m_nodes.end(),[]
								(node_collection_t::value_type& table_item)
								{ table_item.second.get()->update_node_id(); });
//		item.update_node_id();
//		update_storage_node_ids();
	}

	void nsw_tracker::update_node_description(node& item, std::string const& new_descr)
	{
		item.update_node_description(new_descr);
//		update_storage_node_descriptions();
	}

	void nsw_tracker::start(find_data::nodes_callback const& f)
	{
		m_status = started;
		for_each(m_nodes.begin(),m_nodes.end(),[this,&f]
										(node_collection_t::value_type& table_item)
										{
											error_code ec;
											refresh_key(*(table_item.second.get()), ec);
											m_timers_vec.at(table_item.second.get()->nid())
																		.get()->connection_timer.expires_from_now(seconds(1), ec);
											m_timers_vec.at(table_item.second.get()->nid())
																		.get()->connection_timer.async_wait(
														std::bind(&nsw_tracker::connection_timeout
																							, self()
																							, std::ref(*table_item.second)
																							, _1));
											m_timers_vec.at(table_item.second.get()->nid())
																		.get()->refresh_timer.expires_from_now(seconds(5), ec);
											m_timers_vec.at(table_item.second.get()->nid())
																		.get()->refresh_timer.async_wait(std::bind(&nsw_tracker::ping_timeout
																											, self()
																											, std::ref(*table_item.second)
																											, _1));

											table_item.second.get()->bootstrap(f);
//											m_state.clear();
										});
		m_current_callback = &f;
	}

	void nsw_tracker::stop()
	{
		m_status = aborted;
		m_current_callback = nullptr;
		for_each(m_timers_vec.begin(),m_timers_vec.end(),[]
						(std::unordered_map<node_id,std::shared_ptr<timers_t>>::value_type& _item)
						{ 	error_code ec;
						_item.second.get()->key_refresh_timer.cancel(ec);
						_item.second.get()->connection_timer.cancel(ec);
						_item.second.get()->refresh_timer.cancel(ec);});
//		m_key_refresh_timer_vec.cancel(ec);
//		m_connection_timer_vec.cancel(ec);

//		m_refresh_timer.cancel(ec);
//		m_host_resolver.cancel();
	}

	// void nsw_tracker::nsw_status(std::vector<nsw_routing_bucket>& table
	// 	, std::vector<nsw_lookup>& requests)
	// {
	// 	(void)table;
	// 	(void)requests;
	// }

	// void nsw_tracker::update_stats_counters(counters& c) const
	// {
	// 	(void)c;
	// }

	void nsw_tracker::connection_timeout(node& n, error_code const& e)
	{
		if (e || m_status == aborted) return;

		time_duration d = n.connection_timeout();
		error_code ec;

		auto it = m_timers_vec.find(n.nid());

		TORRENT_ASSERT(it != m_timers_vec.end());

		deadline_timer& timer = it->second.get()->connection_timer;
		//m_timers_vec.at(n.nid()).get()->connection_timer;
		timer.expires_from_now(d, ec);
		timer.async_wait(std::bind(&nsw_tracker::connection_timeout, self(), std::ref(n), _1));
	}

	void nsw_tracker::ping_timeout(node& n, error_code const& e)
	{
		if (e || m_status == aborted) return;

		n.tick();

		error_code ec;
		m_timers_vec.at(n.nid()).get()->refresh_timer.expires_from_now(ping_send_timeout, ec);
		m_timers_vec.at(n.nid()).get()->refresh_timer.async_wait(
			std::bind(&nsw_tracker::ping_timeout, self(), std::ref(n), _1));
	}

	void nsw_tracker::refresh_key(node& n, error_code const& e)
	{
		if (e || m_status == aborted) return;

		error_code ec;
		m_timers_vec.at(n.nid()).get()->key_refresh_timer.expires_from_now(key_refresh, ec);
		m_timers_vec.at(n.nid()).get()->key_refresh_timer.async_wait(std::bind(&nsw_tracker::refresh_key, self(), std::ref(n), _1));

		n.new_write_key();

#ifndef TORRENT_DISABLE_LOGGING
		m_log->nsw_log(nsw_logger_interface::tracker, "*** new write key***");
#endif
	}

	// void nsw_tracker::update_storage_node_ids()
	// {
	// }

	void nsw_tracker::get_friends(node& item, sha1_hash const& ih
								, std::string const& target
								, std::function<void(std::multimap<double, tcp::endpoint> const&)> f)
	{
		std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> empty;
		vector_t target_vec;
		term_vector::makeTermVector(target,target_vec);
		item.get_friends(ih, target_vec, f, empty);
	}



	void nsw_tracker::announce(sha1_hash const& ih, int listen_port, int flags
		, std::function<void(std::vector<tcp::endpoint> const&)> f)
	{
		(void)ih;
		(void)listen_port;
		(void)flags;
		(void)f;
	}

//	namespace {

	// void nsw_tracker::direct_request(udp::endpoint const& ep, entry& e
	// 	, std::function<void(msg const&)> f)
	// {
	// 	(void)ep;
	// 	(void)e;
	// 	(void)f;
	// }

	void nsw_tracker::incoming_error(error_code const& ec, udp::endpoint const& ep)
	{
		if (ec == boost::asio::error::connection_refused
			|| ec == boost::asio::error::connection_reset
			|| ec == boost::asio::error::connection_aborted
			)
		{
			for_each(m_nodes.begin(),m_nodes.end(),[&ep]
										(node_collection_t::value_type& table_item)
										{ table_item.second.get()->unreachable(ep); });
		}
	}

	bool nsw_tracker::incoming_packet(udp::endpoint const& ep
		, span<char const> const buf)
	{
		int const buf_size = int(buf.size());
		if (buf_size <= 20
			|| buf.front() != 'd'
			|| buf.back() != 'e') return false;

//		m_counters.inc_stats_counter(counters::nsw_bytes_in, buf_size);
		// account for IP and UDP overhead
//		m_counters.inc_stats_counter(counters::recv_ip_overhead_bytes
//			, ep.address().is_v6() ? 48 : 28);
//		m_counters.inc_stats_counter(counters::nsw_messages_in);

// 		if (m_settings.ignore_dark_internet && ep.address().is_v4())
// 		{
// 			address_v4::bytes_type b = ep.address().to_v4().to_bytes();

// 			// these are class A networks not available to the public
// 			// if we receive messages from here, that seems suspicious
// 			static std::uint8_t const class_a[] = { 3, 6, 7, 9, 11, 19, 21, 22, 25
// 				, 26, 28, 29, 30, 33, 34, 48, 51, 56 };

// 			if (std::find(std::begin(class_a), std::end(class_a), b[0]) != std::end(class_a))
// 			{
// //				m_counters.inc_stats_counter(counters::dht_messages_in_dropped);
// 				return true;
// 			}
// 		}

		// if (!m_blocker.incoming(ep.address(), clock_type::now(), m_log))
		// {
		// 	m_counters.inc_stats_counter(counters::dht_messages_in_dropped);
		// 	return true;
		// }

		TORRENT_ASSERT(buf_size > 0);

		int pos;
		error_code err;
		int ret = bdecode(buf.data(), buf.data() + buf_size, m_msg, err, &pos, 10, 500);
		if (ret != 0)
		{
//			m_counters.inc_stats_counter(counters::nsw_messages_in_dropped);
#ifndef TORRENT_DISABLE_LOGGING
			m_log->nsw_log_packet(nsw_logger_interface::incoming_message, buf, ep);
#endif
			return false;
		}

		if (m_msg.type() != bdecode_node::dict_t)
		{
//			m_counters.inc_stats_counter(counters::nsw_messages_in_dropped);
#ifndef TORRENT_DISABLE_LOGGING
			m_log->nsw_log_packet(nsw_logger_interface::incoming_message, buf, ep);
#endif
			// it's not a good idea to send a response to an invalid messages
			return false;
		}

#ifndef TORRENT_DISABLE_LOGGING
		m_log->nsw_log_packet(nsw_logger_interface::incoming_message, buf, ep);
#endif

		libtorrent::nsw::msg m(m_msg, ep);
		std::for_each(m_nodes.begin(), m_nodes.end(), [this, &m]
													(node_collection_t::value_type& table_item)
													{ table_item.second.get()->incoming(m); });
//		m_nsw_templ.incoming(m);
		return true;
	}

	namespace {

	void add_node_fun(void* userdata, node_entry const& e)
	{
		(void*)userdata;
		(void)e;
	}

	std::vector<udp::endpoint> save_nodes(node const& nsw)
	{
		std::vector<udp::endpoint> ret;
		(void)nsw;
		return ret;
	}

} // anonymous namespace

	void nsw_tracker::add_node(sha1_hash const& nid, std::string const& description)
	{
		if (m_nodes.find(description) == m_nodes.end())
		{


			std::shared_ptr<node> nodeRef(new node(udp::v4()
										,this
										,m_settings
										,nid
										,description
										,dynamic_cast<nsw_logger_observer_interface*>(m_log)
										,m_counters));

			nodeRef.get()->update_node_id();
			m_nodes[description] = nodeRef;

			//pass gateway nodes
			std::for_each(m_nsw_gate_nodes.begin(), m_nsw_gate_nodes.end(), [&nodeRef]
													(std::pair<vector_t, udp::endpoint> const& item)
													{nodeRef.get()->add_gate_node(item.second, item.first);});


			std::shared_ptr<timers_t> timerRef(new timers_t(m_io_srv));
//			timers_t node_timers(m_io_srv);
			m_timers_vec.insert(std::make_pair(nodeRef.get()->nid(),timerRef));

			if(m_status == started)
			{
				error_code ec;
				refresh_key(*(nodeRef), ec);
				m_timers_vec.at(nodeRef->nid()).get()->connection_timer.expires_from_now(seconds(1), ec);
				m_timers_vec.at(nodeRef->nid()).get()->connection_timer.async_wait(
							std::bind(&nsw_tracker::connection_timeout
																, self()
																, std::ref(*nodeRef)
																, _1));
				m_timers_vec.at(nodeRef->nid()).get()->refresh_timer.expires_from_now(seconds(5), ec);
				m_timers_vec.at(nodeRef->nid()).get()->refresh_timer.async_wait(std::bind(&nsw_tracker::ping_timeout
																				, self()
																				, std::ref(*nodeRef)
																				, _1));

				nodeRef->bootstrap(*m_current_callback);
			}
		}
	}

	void nsw_tracker::add_gate_node(udp::endpoint const& node
									, std::string const& description)
	{
		//pass gate item for each node

		vector_t gate_desription_vec;
		term_vector::makeTermVector(description,gate_desription_vec);
		for_each(m_nodes.begin(),m_nodes.end(),[&node,&gate_desription_vec]
										(node_collection_t::value_type& table_item)
										{ table_item.second.get()->add_gate_node(node,gate_desription_vec); });

		m_nsw_gate_nodes.push_back(std::make_pair(gate_desription_vec, node));

	}

	// bool nsw_tracker::has_quota()
	// {
	// 	return false;
	// }

	bool nsw_tracker::send_packet(entry& e, udp::endpoint const& addr)
	{
		static char const version_str[] = {'L', 'T', 0, 1};
		e["v"] = std::string(version_str, version_str + 4);

		m_send_buf.clear();
		bencode(std::back_inserter(m_send_buf), e);

		// update the quota. We won't prevent the packet to be sent if we exceed
		// the quota, we'll just (potentially) block the next incoming request.

		//m_send_quota -= int(m_send_buf.size());

		error_code ec;
		m_send_fun(addr, m_send_buf, ec, 0);
		if (ec)
		{
//			m_counters.inc_stats_counter(counters::nsw_messages_out_dropped);
#ifndef TORRENT_DISABLE_LOGGING
			m_log->nsw_log_packet(nsw_logger_interface::outgoing_message, m_send_buf, addr);
#endif
			return false;
		}

		// m_counters.inc_stats_counter(counters::dht_bytes_out, m_send_buf.size());
		// // account for IP and UDP overhead
		// m_counters.inc_stats_counter(counters::sent_ip_overhead_bytes
		// 	, addr.address().is_v6() ? 48 : 28);
		// m_counters.inc_stats_counter(counters::dht_messages_out);
#ifndef TORRENT_DISABLE_LOGGING
		m_log->nsw_log_packet(nsw_logger_interface::outgoing_message, m_send_buf, addr);
#endif
		return true;
	}

}}

