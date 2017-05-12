#ifndef LIBTORRENT_GET_PEERS_HPP
#define LIBTORRENT_GET_PEERS_HPP

#include "libtorrent/navigable_small_world/find_data.hpp"
#include <cstdio>
namespace libtorrent { namespace nsw
{

class get_friends : public find_data
{
public:
	typedef std::function<void(std::vector<std::tuple<node_id, udp::endpoint, std::string>> const&)> data_callback;

    struct row
    {
        node_id id;
        udp::endpoint addr;
        std::string token;
        std::string transaction_id;
        double simil;
        bool is_visited;
        row():id(0)
        	,addr()
        	,token("")
        	,transaction_id("")
        	,simil(0.0)
        	,is_visited(false)
        {}

        row(node_id const& a_id
        				,udp::endpoint const& a_addr
        				,std::string const& a_token
        				,std::string const& a_trans_id
        				,double& a_simil
        				,bool a_visited):id(a_id)
        								,addr(a_addr)
							        	,token(a_token)
							        	,transaction_id(a_trans_id)
							        	,simil(a_simil)
							        	,is_visited(a_visited)
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

public:
	void got_friends(/*std::multimap<double, tcp::endpoint> const& friends*/);

	get_friends(node& nsw_node
		, vector_t const& target
		, data_callback const& dcallback
		, nodes_callback const& ncallback);

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
		, vector_t const& text)
		: find_data_observer(algorithm, ep, id, text)
	{}

	virtual void reply(msg const&);

#ifndef TORRENT_DISABLE_LOGGING
	void log_friends(msg const& m, bdecode_node const& r, int const size) const;
#endif
};

} } // namespace libtorrent::nsw

#endif // LIBTORRENT_GET_PEERS_HPP
