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
    int lvl = r.dict_find_int("q_lvl").int_value();
    int lay = r.dict_find_int("q_lay").int_value();
    algorithm()->increase_visited();
    bdecode_node q_id = r.dict_find_string("q_id");

    //if(q_id)
    node_id checked_id(q_id.string_ptr());

//  std::lock_guard<std::mutex> l(m_mutex);

    auto checked_friend = std::find_if(algo_ptr->candidates.begin(),algo_ptr->candidates.end(),[&checked_id](get_friends::row const& item)
                                                                                {return checked_id == item.id;});
    if(checked_friend == algo_ptr->candidates.end())
    {
        //possibly it is a gate and it had id = 0
        checked_friend = std::find_if(algo_ptr->candidates.begin(),algo_ptr->candidates.end(),[&transaction](get_friends::row const& item)
                                                                                {return transaction == item.transaction_id;});
    }


    TORRENT_ASSERT(checked_friend != algo_ptr->candidates.end());

    get_friends::row tmp(*checked_friend);

    bdecode_node me = r.dict_find_string("me");
    if (me)
    {
        double term_value = 0.0;
        std::memcpy(&term_value, std::string(me.string_value()).c_str(), sizeof(double));
        tmp.simil = term_value;
        // add for add_friend algorithm()->
    }

    tmp.visit_status = get_friends::row::visited;
    if(tmp.id.is_all_zeros())
        tmp.id = checked_id;

    bdecode_node token = r.dict_find_string("token");
    if (token)
    {
        tmp.token = token.string_value().to_string();
    }

    algo_ptr->candidates.erase(checked_friend);
    algo_ptr->candidates.insert(tmp);
    if(exact_search && me || !exact_search)
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
            else if(node.id == algo_ptr->get_node().nid())
            {
                // some one have rocemmented us
                // leave as separate case for future actions
                continue;
            }
            else
            {
                algo_ptr->candidates.emplace(node.id, node.ep, "", "", node.similarity, lvl, lay, get_friends::row::not_visited, tmp.generation+1);
            }
        }
    }

    algo_ptr->got_friends();
    find_data_observer::reply(m);
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
void get_friends::got_friends()
{
    //bad practice. need to implement as low granulate locks
    std::lock_guard<std::mutex> l(m_mutex);

    if(!appropriate_candidate_exists()
        || candidates.empty())
    {

        get_friends::callback_data_t callback_data;

        std::for_each(friend_list.begin(), friend_list.end(), [&callback_data](row const& item)
                                        {
                                            callback_data.push_back(std::make_tuple(item.id
                                                                                    , item.addr
                                                                                    , item.token
                                                                                    , item.simil
                                                                                    , item.generation
                                                                                    , item.level
                                                                                    , item.layer));
                                        });
        if (invoke_count() == 1 && m_data_callback)
        {
            nsw_lookup common_statistic;
            status(common_statistic);

            m_data_callback(callback_data,common_statistic);
            find_data::done();
        }
    }
    else
    {
        auto next_candidate = std::find_if(candidates.begin(),candidates.end(),[](get_friends::row const& item)
                                                                            {return item.visit_status == row::not_visited; });

        ask_next_node(next_candidate);
    }
}

get_friends::get_friends(
    node& nsw_node
    , vector_t const& target
    , data_callback const& dcallback
    , nodes_callback const& ncallback
    , int lvl
    , int lay)
    : find_data(nsw_node, nsw_node.nid(),target, ncallback)
    , m_data_callback(dcallback)
    , m_lvl(lvl)
    , m_lay(lay)
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
    a["q_lvl"] = m_lvl;
    a["q_lay"] = m_lay;
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
    TORRENT_ASSERT(current_row != candidates.end());
    //current_row.visit_status = get_friends::row::not_visited;
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
                                                                    return item.visit_status == get_friends::row::not_visited &&
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
                                                                {return item.visit_status == get_friends::row::not_visited;});

        if (exists == candidates.end())
            return false;

    }

    return true;
}

void get_friends::ask_next_node(friends_results_table::iterator const& it)
{
    auto next_candidate_observer = get_friends::new_observer(it->addr,it->id,target());

    m_results.push_back(next_candidate_observer);
    add_requests();
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
    double term_value = 0.0;
    int lvl, lay;

    std::copy(str_in, str_in + sizeof(node_id), n.id.begin());
    str_in += sizeof(node_id);

    std::copy(str_in, str_in + sizeof(double), (char*)&term_value);

    n.similarity = term_value;
    str_in += sizeof(double);
    std::copy(str_in, str_in + sizeof(int), (char*)&lvl);
    n.level = lvl;
    str_in += sizeof(int);
    std::copy(str_in, str_in + sizeof(int), (char*)&lay);
    n.layer = lay;
    str_in += sizeof(int);
    n.ep = detail::read_v4_endpoint<udp::endpoint>(str_in);
    return n;
}

} } // namespace libtorrent::nsw
