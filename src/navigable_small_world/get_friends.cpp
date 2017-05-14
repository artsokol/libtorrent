#include "libtorrent/navigable_small_world/get_friends.hpp"
#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/socket_io.hpp"
#include "libtorrent/performance_counters.hpp"

#ifndef TORRENT_DISABLE_LOGGING
#include <libtorrent/hex.hpp> // to_hex
#endif

namespace libtorrent { namespace nsw
{

void get_friends_observer::reply(msg const& m)
{
	get_friends* algo_ptr = static_cast<get_friends*>(algorithm());
	int k = algo_ptr->get_node().m_table.neighbourhood_size();
	std::string transaction = m.message.dict_find_string_value("t").to_string();

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

	bdecode_node q_id = r.dict_find_string("q_id");

	//if(q_id)
	node_id checked_id(q_id.string_ptr());

	auto checked_friend = std::find_if(algo_ptr->candidates.begin(),algo_ptr->candidates.end(),[&checked_id](get_friends::row const& item)
																				{return checked_id == item.id;});
	if(checked_friend == algo_ptr->candidates.end())
	{
		//possibly it is a gate and it had id = 0

		//int tr_id = algo_ptr->get_node().m_table.get_my_transaction_id(this);
		checked_friend = std::find_if(algo_ptr->candidates.begin(),algo_ptr->candidates.end(),[&transaction](get_friends::row const& item)
																				{return transaction == item.transaction_id;});

	}


    TORRENT_ASSERT(checked_friend != algo_ptr->candidates.end());

	get_friends::row tmp(*checked_friend);

	bdecode_node me = r.dict_find_int("me");
	if (me)
	{
		std::string received_num_str =  "0." + std::string(me.string_value());
		tmp.simil = std::stod(received_num_str);
		// add for add_friend algorithm()->
	}

 	tmp.is_visited = true;
 	if(tmp.id.is_all_zeros())
 		tmp.id = checked_id;

	bdecode_node token = r.dict_find_string("token");
	if (token)
	{
		tmp.token = token.string_value().to_string();
	}

	algo_ptr->candidates.erase(checked_friend);
	algo_ptr->candidates.insert(tmp);
	algo_ptr->friend_list.insert(tmp);


	// look for friends
	bdecode_node n = r.dict_find_list("friends");
	if (n)
	{
		for(int i = 0; i < n.list_size(); ++i)
		{
			node_id friend_id;
			udp::endpoint addr;
			char const* elem = n.list_at(i).string_ptr();

			get_friends::node_item node = get_friends::read_node_item(elem);

			auto exists = std::find_if(algo_ptr->candidates.begin(),algo_ptr->candidates.end(),[&node](get_friends::row const& item)
																				{return node.id == item.id;});
			if(exists != algo_ptr->candidates.end())
			{
				continue;
			}
			else
			{
				algo_ptr->candidates.emplace(node.id,node.ep,"","",node.similarity,false);
			}
		}
	}



	algo_ptr->got_friends();
	find_data_observer::reply(m);

			//static_cast<get_friends*>(algorithm())->invoke(next_observer);


			//static_cast<get_friends*>(algorithm())->friend_list.emplace(friend_id,addr,similarity,false);
			//add_entry(friend_id, addr, similarity, observer_interface::flag_initial);
		//}

// #ifndef TORRENT_DISABLE_LOGGING
// 			log_friends(m, r, n.list_size());
// #endif

	//}


}

#ifndef TORRENT_DISABLE_LOGGING
void get_friends_observer::log_friends(msg const& m, bdecode_node const& r, int const size) const
{
			auto logger = get_observer();
			if (logger != nullptr && logger->should_log(nsw_logger_interface::traversal))
			{
				bdecode_node const id = r.dict_find_string("r_id");
				if (id && id.string_length() == 20)
				{
					logger->nsw_log(nsw_logger_interface::traversal, "[%u] FRIENDS "
						"invoke-count: %d branch-factor: %d addr: %s id: %sp: %d"
						, algorithm()->id()
						, algorithm()->invoke_count()
						, algorithm()->branch_factor()
						, print_endpoint(m.addr).c_str()
						, aux::to_hex({id.string_ptr(), size_t(id.string_length())}).c_str()
						, size);
				}
			}
}
#endif
void get_friends::got_friends(/*std::multimap<double, tcp::endpoint> const& friends*/)
{
	if(!appropriate_candidate_exists()
		|| candidates.empty())
	{

		std::vector<std::tuple<node_id, udp::endpoint, std::string>> callback_data;

		std::for_each(friend_list.begin(), friend_list.end(), [&callback_data](row const& item)
										{callback_data.push_back(std::make_tuple(item.id, item.addr, item.token)); }
										);

		if (m_data_callback) m_data_callback(callback_data);
		traversal_algorithm::done();
	}
	else
	{
		auto next_candidate = std::find_if(candidates.begin(),candidates.end(),[](get_friends::row const& item)
																			{return item.is_visited == false; });

		ask_next_node(next_candidate);
		//candidates.erase(candidates.begin());
	}



}

get_friends::get_friends(
	node& nsw_node
	, vector_t const& target
	, data_callback const& dcallback
	, nodes_callback const& ncallback)
	: find_data(nsw_node, nsw_node.nid(),target, ncallback)
	, m_data_callback(dcallback)
{
}

char const* get_friends::name() const { return "get_friends"; }

bool get_friends::invoke(observer_ptr o)
{
	if (m_done) return false;
	std::string transaction;
	entry e;
	e["y"] = "q";
	entry& a = e["a"];

	e["q"] = "get_friends";
	a["r_id"] = o->id().to_string();
	entry& term_vec = a["term_vector"];
	term_vector::vectorToEntry(target(),term_vec);

	if (m_node.observer() != nullptr)
	{
		m_node.observer()->outgoing_get_friends(target_nid(), term_vector::vectorToString(target()), o->target_ep());
	}

	m_node.stats_counters().inc_stats_counter(counters::nsw_get_friends_out);

	bool ret = m_node.m_rpc.invoke(e, o->target_ep(), o, transaction);

	auto current_row = std::find_if(candidates.begin(), candidates.end(), [&o](row const& item)
																			{return o->id() == item.id;});

	if (current_row->transaction_id == "")
		// temp solution. bad practice
		const_cast<std::string&>(current_row->transaction_id) = transaction;

	return ret;
}

bool get_friends::appropriate_candidate_exists()
{
	int k = m_node.m_table.neighbourhood_size();
	int i = k;

	if(friend_list.size()>=k)
	{
		auto it = friend_list.begin();

		// move iterator to i-th position
		while(--i>0) ++it;

		auto exists = std::find_if(candidates.begin(),candidates.end(),[&it](row const& item)
																{
																	return item.is_visited == false &&
																		it->simil < item.simil;
																});
		// stop condition. needed to be updated
		if (exists == candidates.end() /*|| friend_list.size() > k*/)
			return false;
	}
	else
	{
		// friend list very small. Any not visited nodes are good
		auto exists = std::find_if(candidates.begin(),candidates.end(),[](row const& item)
																{return item.is_visited == false;});

		if (exists == candidates.end())
			return false;

	}

	return true;
}

void get_friends::ask_next_node(friends_results_table::iterator const& it)
{
	auto next_candidate_observer = get_friends::new_observer(it->addr,it->id,target());

	m_results.push_back(next_candidate_observer);
	dynamic_cast<get_friends*>(this)->add_requests();
}

observer_ptr get_friends::new_observer(udp::endpoint const& ep
	, node_id const& id
	, vector_t const& text)
{
	auto o = m_node.m_rpc.allocate_observer<get_friends_observer>(self(), ep, id, text);

	return o;
}

get_friends::node_item get_friends::read_node_item(char const* str_in)
{
	get_friends::node_item n;
	std::string similsrity_str = "0.";
	int similarity_offset = 7;
	std::copy(str_in, str_in + sizeof(node_id), n.id.begin());
	str_in += sizeof(node_id);

	std::copy(str_in, str_in + similarity_offset, similsrity_str.begin()+2);

	unsigned int received_num = std::stoi(similsrity_str);
	n.similarity = std::stod(similsrity_str);

	str_in += similarity_offset;
	n.ep = detail::read_v4_endpoint<udp::endpoint>(str_in);
	return n;
}

} } // namespace libtorrent::nsw
