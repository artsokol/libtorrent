#ifndef BOOTSTRAP_HPP
#define BOOTSTRAP_HPP

#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/get_friends.hpp"

namespace libtorrent { namespace nsw
{

class bootstrap : public get_friends
{
public:
	typedef find_data::nodes_callback done_callback;
    //const replacement_table_t::value_type& table_item
	bootstrap(node& nsw_node
        , node_id const& nid
        , vector_t const& target_text
        , done_callback const& callback
        , data_callback const& result_callback
        , bool exact = false);
	virtual char const* name() const;

	observer_ptr new_observer(udp::endpoint const& ep
		, node_id const& id
        , vector_t const& text);

	void trim_seed_nodes();


protected:

	virtual bool invoke(observer_ptr o);

	virtual void done();
private:
    bool m_exact_search;

};

} } // namespace libtorrent::nsw

#endif // BOOTSTRAP_HPP
