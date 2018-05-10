#ifndef LIBTORRENT_GET_PEERS_HPP
#define LIBTORRENT_GET_PEERS_HPP

#include "libtorrent/navigable_small_world/find_data.hpp"
#include <cstdio>
#include <mutex>          // std::mutex
namespace libtorrent { namespace nsw
{

class get_friends : public find_data
{
public:
    typedef std::vector<std::tuple<node_id, udp::endpoint, std::string, double, uint16_t>> callback_data_t;
    typedef std::function<void(callback_data_t const&, nsw_lookup const& common_statistic/*, int , int*/)> data_callback;

    struct row
    {
        typedef enum {not_visited = 0, visiting, visited} visiting_status_t;
        node_id id;
        udp::endpoint addr;
        std::string token;
        std::string transaction_id;
        double simil;
        visiting_status_t visit_status;
        uint16_t generation;
        int level;
        int layer;
        row():id(0)
            ,addr()
            ,token("")
            ,transaction_id("")
            ,simil(0.0)
            ,visit_status(not_visited)
            ,generation(0)
            ,level(0)
            ,layer(0)
        {}

        row(node_id const& a_id
            ,udp::endpoint const& a_addr
            ,std::string const& a_token
            ,std::string const& a_trans_id
            ,double& a_simil
            ,int lvl
            ,int lay
            ,visiting_status_t a_visited
            ,uint16_t a_generation=0)
        :id(a_id)
        ,addr(a_addr)
        ,token(a_token)
        ,transaction_id(a_trans_id)
        ,simil(a_simil)
        ,visit_status(a_visited)
        ,generation(a_generation)
        ,layer(lay)
        ,level(lvl)
        {}

        bool operator==(row const& r)
        {
            return this->id == r.id;
        }

    };

    struct node_item
    {
        node_id id;
        udp::endpoint ep;
        double similarity;
        int level;
        int layer;
    };

    class rowComp {
    public:
        bool operator()(row const& a, row const& b) {

            return a.simil > b.simil;
        }
    };

    using friends_results_table = std::multiset<row,rowComp>;

    friends_results_table friend_list;
    friends_results_table candidates;
protected:
    data_callback m_data_callback;
    std::mutex m_mutex;
    int m_lvl;
    int m_lay;

public:
    void got_friends(/*std::multimap<double, tcp::endpoint> const& friends*/);

    get_friends(node& nsw_node
        , vector_t const& target
        , data_callback const& dcallback
        , nodes_callback const& ncallback
        // , int lvl
        // , int lay
        );

    virtual char const* name() const;

    bool appropriate_candidate_exists();
    void ask_next_node(friends_results_table::iterator const& it);

    static node_item read_node_item(char const* str);
protected:
    virtual bool invoke(observer_ptr o);
    virtual observer_ptr new_observer(udp::endpoint const& ep
        , node_id const& id
        , vector_t const& text);
};


struct get_friends_observer : find_data_observer
{
    get_friends_observer(
        std::shared_ptr<traversal_algorithm> const& algorithm
        , udp::endpoint const& ep
        , node_id const& id
        , vector_t const& text
        , bool exact = false)
        : find_data_observer(algorithm, ep, id, text)
        , exact_search(exact)
    {}

    bool exact_search;
    virtual void reply(msg const&);

#ifndef TORRENT_DISABLE_LOGGING
    void log_friends(msg const& m, bdecode_node const& r, int const size) const;
#endif
};

} } // namespace libtorrent::nsw

#endif // LIBTORRENT_GET_PEERS_HPP
