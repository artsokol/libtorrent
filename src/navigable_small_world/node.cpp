#include "libtorrent/config.hpp"

#include <utility>
#include <cinttypes>
#include <functional>
#include <tuple>
#include <array>

#ifndef TORRENT_DISABLE_LOGGING
#include "libtorrent/hex.hpp"
#endif

#include "libtorrent/socket_io.hpp"
#include "libtorrent/session_status.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/random.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/aux_/time.hpp"
#include "libtorrent/alert_types.hpp"
#include "libtorrent/performance_counters.hpp"

//use kademlia header
#include "libtorrent/kademlia/io.hpp"

#include "libtorrent/navigable_small_world/node.hpp"
#include "libtorrent/navigable_small_world/nsw_tracker.hpp"
#include "libtorrent/navigable_small_world/bootstrap.hpp"
#include "libtorrent/navigable_small_world/nsw_logger_observer_interface.hpp"
#include "libtorrent/navigable_small_world/term_vector.hpp"

namespace libtorrent { namespace nsw {

namespace {

//void nop() {}

node_id calculate_node_id(node_id const& nid, nsw_logger_observer_interface* observer, udp protocol)
{
    address external_address;
    if (observer != nullptr) external_address = observer->external_nsw_address(protocol);

    // if we don't have an observer, don't pretend that external_address is valid
    // generating an ID based on 0.0.0.0 would be terrible. random is better
    if (observer == nullptr || external_address.is_unspecified())
    {
        return generate_random_id();
    }

    if (nid == node_id::min() || !verify_id(nid, external_address))
        return generate_id(external_address);

    return nid;
}

void incoming_error(entry& e, char const* msg, uint32_t error_code = 201)
{
    e["y"] = "e";
    entry::list_type& l = e["e"].list();
    l.push_back(entry(error_code));
    l.push_back(entry(msg));
}
} // anonymous namespace


node::node(udp proto, udp_socket_interface* sock
    , nsw_settings const& settings
    , node_id const& nid
    , std::string const& description
    , nsw_logger_observer_interface* observer
    , counters& cnt)
    : m_settings(settings)
    , m_id(calculate_node_id(nid, observer, proto))
    , m_description(description)
    , m_table(m_id, proto, 8, settings, description, observer)
    , m_rpc(m_id, m_settings, m_table, sock, observer)
//  , m_nodes(nodes)
    , m_observer(observer)
    , m_protocol(map_protocol_to_descriptor(proto))
    , m_last_tracker_tick(aux::time_now())
    , m_last_self_refresh(min_time())
    , m_sock(sock)
    , m_counters(cnt)
    , m_is_query_running(false)
{
    m_secret[0] = random(~0);
    m_secret[1] = random(~0);
}

node::~node() = default;

void node::update_node_id()
{
    // if we don't have an observer, we can't ask for the external IP (and our
    // current node ID is likely not generated from an external address), so we
    // can just stop here in that case.
    if (m_observer == nullptr) return;

    // it's possible that our external address hasn't actually changed. If our
    // current ID is still valid, don't do anything.
    if (verify_id(m_id, m_observer->external_nsw_address(get_protocol())))
        return;

#ifndef TORRENT_DISABLE_LOGGING
    m_observer->nsw_log(nsw_logger_interface::node
        , "updating node ID (because external IP address changed)");
#endif

    m_id = generate_id(m_observer->external_nsw_address(get_protocol()));
    // most likely we shoudl decide about changing id case
    m_table.update_node_id(m_id);
    m_rpc.update_node_id(m_id);
}

void node::update_node_description(std::string const& new_descr)
{

}

bool node::verify_token(string_view token, sha1_hash const& info_hash
    , udp::endpoint const& addr) const
{
    if (token.length() != 4)
    {
#ifndef TORRENT_DISABLE_LOGGING
        if (m_observer != nullptr)
        {
            m_observer->nsw_log(nsw_logger_interface::node, "token of incorrect length: %d"
                , int(token.length()));
        }
#endif
        return false;
    }

    hasher h1;
    error_code ec;
    std::string const address = addr.address().to_string(ec);
    if (ec) return false;
    h1.update(address);
    h1.update(reinterpret_cast<char const*>(&m_secret[0]), sizeof(m_secret[0]));
    h1.update(info_hash);

    sha1_hash h = h1.final();
    if (std::equal(token.begin(), token.end(), reinterpret_cast<char*>(&h[0])))
        return true;

    hasher h2;
    h2.update(address);
    h2.update(reinterpret_cast<char const*>(&m_secret[1]), sizeof(m_secret[1]));
    h2.update(info_hash);
    h = h2.final();
    return std::equal(token.begin(), token.end(), reinterpret_cast<char*>(&h[0]));
}

std::string node::generate_token(udp::endpoint const& addr, sha1_hash const& info_hash)
{
    std::string token;
    token.resize(4);
    hasher h;
    error_code ec;
    std::string const address = addr.address().to_string(ec);
    h.update(address);
    h.update(reinterpret_cast<char*>(&m_secret[0]), sizeof(m_secret[0]));
    h.update(info_hash);

    sha1_hash const hash = h.final();
    std::copy(hash.begin(), hash.begin() + 4, token.begin());
    return token;
}


void node::status(std::vector<std::pair<int, std::vector<node_entry>>>& cf_table)
{

    std::lock_guard<std::mutex> l(m_mutex);

    m_table.status(cf_table);
}

void node::bootstrap(/*std::vector<udp::endpoint> const& nodes,*/
        find_data::nodes_callback const& f)
{
    auto r = std::make_shared<nsw::bootstrap>(*this
                                            , m_id
                                            , m_table.get_descr()
                                            , f
                                            , std::bind(&node::add_friend_engine, this, _1, _2));

    m_last_self_refresh = aux::time_now();

#ifndef TORRENT_DISABLE_LOGGING
    int count = 0;
#endif
    //add closest friends

    auto& table = m_table.neighbourhood();
    for (int it = 0; it !=table.size(); ++it) {
        int lay = table[it].first;
        for_each(table[it].second.begin(), table[it].second.end(), [this, &r, &count, it, lay] (const node_entry& n) {
#ifndef TORRENT_DISABLE_LOGGING
            ++count;
#endif
            r->add_entry(n.id, m_table.get_descr(), n.endpoint, observer_interface::flag_initial);
        });
    }


    // for_each(m_table.neighbourhood().begin(),m_table.neighbourhood().end(), [](const std::pair<int, std::vector<node_entry>>& item) {
    //  for_each(item.second.begin(),item.second.end(),[this, &r, &count, &item]
    //                                      (const node_entry& n)
    //                                  {
    //                              #ifndef TORRENT_DISABLE_LOGGING
    //                                          ++count;
    //                              #endif
    //                                          r->add_entry(n.id, m_table.get_descr(), n.endpoint, observer_interface::flag_initial, item.fi);
    //                                  });

    // });
    // //add old friends
    // for_each(m_table.old_relations().begin(),m_table.old_relations().end(),[this, &r, &count]
    //                                  (const node_entry& n)
    //                              {
    //                          #ifndef TORRENT_DISABLE_LOGGING
    //                                      ++count;
    //                          #endif
    //                                      r->add_entry(n.id, m_table.get_descr(), n.endpoint, observer_interface::flag_initial);
    //                              });

    // make us start as far away from our node ID as possible
    r->trim_seed_nodes();

#ifndef TORRENT_DISABLE_LOGGING
    if (m_observer != nullptr)
        m_observer->nsw_log(nsw_logger_interface::node, "bootstrapping with %d nodes", count);
#endif
    r->start();

}

void node::nsw_query(vector_t& query_vec)
{
    // check if we most similar to query
    if(m_is_query_running)
        return;

    m_is_query_running = true;
    m_query_results.clear();

    std::vector<node_entry> closest_friends;
    m_table.find_node(query_vec, closest_friends, m_table.neighbourhood_size(), 0);

    double similarity_with_me = term_vector::getVecSimilarity(m_table.get_descr(), query_vec);
    double best_friend_similarity = (closest_friends.size() >0) ?
                            term_vector::getVecSimilarity(closest_friends.front().term_vector, query_vec):
                            0.0;

    if (best_friend_similarity < similarity_with_me)
    {
        std::lock_guard<std::mutex> l(m_mutex);
        vector_t description_vec;
        term_vector::makeTermVector(m_description,description_vec);
        m_query_results.insert(std::make_pair(term_vector::getVecSimilarity(query_vec,description_vec), m_description));
        return;
    }

    auto finalization_actions = [this](
            std::vector<std::pair<nsw::node_entry, std::string>> const&)
        {
        };

    auto r = std::make_shared<nsw::bootstrap>(*this
                                            , m_id
                                            , query_vec
                                            , finalization_actions
                                            , std::bind(&node::nsw_query_engine, this, _1, _2)
                                            , true);
    m_last_self_refresh = aux::time_now();

#ifndef TORRENT_DISABLE_LOGGING
    int count = 0;
#endif
    //add closest friends

    auto& table = m_table.neighbourhood();
    for (int lvl = 0; lvl != table.size(); ++lvl) {
        int lay = table[lvl].first;
        for(auto& n : table[lvl].second) {
#ifndef TORRENT_DISABLE_LOGGING
            ++count;
#endif
            r->add_entry(n.id, m_table.get_descr(), n.endpoint, observer_interface::flag_initial);          
        }
    }


#ifndef TORRENT_DISABLE_LOGGING
    if (m_observer != nullptr)
        m_observer->nsw_log(nsw_logger_interface::node, "query %s with %d nodes", term_vector::vectorToString(query_vec).c_str(), count);
#endif
    r->start();
}

void node::new_write_key()
{
    m_secret[1] = m_secret[0];
    m_secret[0] = random(~0);
}

void node::unreachable(udp::endpoint const& ep)
{
    m_rpc.unreachable(ep);
}

void node::incoming(msg const& m)
{
    // is this a reply?
    bdecode_node y_ent = m.message.dict_find_string("y");
    if (!y_ent || y_ent.string_length() == 0)
    {
        // don't respond to this obviously broken messages. We don't
        // want to open up a magnification opportunity
        return;
    }

    char y = *(y_ent.string_ptr());

    bdecode_node ext_ip = m.message.dict_find_string("ip");

    // backwards compatibility
    if (!ext_ip)
    {
        bdecode_node r = m.message.dict_find_dict("r");
        if (r)
            ext_ip = r.dict_find_string("ip");
    }

#if TORRENT_USE_IPV6
    if (ext_ip && ext_ip.string_length() >= 16)
    {
        // this node claims we use the wrong node-ID!
        address_v6::bytes_type b;
        std::memcpy(&b[0], ext_ip.string_ptr(), 16);
        if (m_observer != nullptr)
            m_observer->set_nsw_external_address(address_v6(b)
                , m.addr.address());
    } else
#endif
    if (ext_ip && ext_ip.string_length() >= 4)
    {
        address_v4::bytes_type b;
        std::memcpy(&b[0], ext_ip.string_ptr(), 4);
        if (m_observer != nullptr)
            m_observer->set_nsw_external_address(address_v4(b)
                , m.addr.address());
    }

    switch (y)
    {
        case 'r':
        {
            node_id id;
            m_rpc.incoming(m, &id);
            break;
        }
        case 'q':
        {
            TORRENT_ASSERT(m.message.dict_find_string_value("y") == "q");

            if (!native_address(m.addr)) break;

            entry e;
            incoming_request(m, e);
            m_sock->send_packet(e, m.addr);
            break;
        }
        case 'e':
        {
#ifndef TORRENT_DISABLE_LOGGING
            if (m_observer != nullptr && m_observer->nsw_should_log(nsw_logger_interface::node))
            {
                bdecode_node err = m.message.dict_find_list("e");
                if (err && err.list_size() >= 2
                    && err.list_at(0).type() == bdecode_node::int_t
                    && err.list_at(1).type() == bdecode_node::string_t)
                {
                    m_observer->nsw_log(nsw_logger_interface::node, "INCOMING ERROR: (%" PRId64 ") %s"
                        , err.list_int_value_at(0)
                        , err.list_string_value_at(1).to_string().c_str());
                }
                else
                {
                    m_observer->nsw_log(nsw_logger_interface::node, "INCOMING ERROR (malformed)");
                }
            }
#endif
            node_id id;
            m_rpc.incoming(m, &id);
            break;
        }
    }
}

void node::add_gate_node(udp::endpoint const& gate, vector_t const& description_vec)
{
#ifndef TORRENT_DISABLE_LOGGING
    if (m_observer != nullptr && m_observer->nsw_should_log(nsw_logger_interface::node))
    {
        m_observer->nsw_log(nsw_logger_interface::node, "adding gate node: %s"
            , print_endpoint(gate).c_str());
    }
#endif
    m_table.add_gate_node(gate,description_vec);
}

// void node::add_node(udp::endpoint const& node, node_id const& id)
// {
//  if (!native_address(node)) return;
//  // ping the node, and if we get a reply, it
//  // will be added to the routing table

//  send_ping(node, id);
//  //m_table.add_node(node);
// }

// void node::get_friends(sha1_hash const& info_hash
//      , vector_t const& target
//      , std::function<void(std::vector<std::tuple<node_id, udp::endpoint, std::string, double>> const&)> dcallback
//      , std::function<void(std::vector<std::pair<node_entry, std::string>> const&)> ncallback)
// {
//  // search for nodes with text close to our.
//  auto ta = std::make_shared<nsw::get_friends>(*this, target, dcallback, ncallback);

//  ta->start();
// }

void node::add_friend(sha1_hash const& info_hash, int listen_port, int flags
        , std::function<void(std::vector<tcp::endpoint> const&)> f)
{

}

class ping_observer : public observer_interface
{
public:
    ping_observer(
        std::shared_ptr<traversal_algorithm> const& algorithm
        , udp::endpoint const& ep, node_id const& id)
        : observer_interface(algorithm, ep, id, vector_t())
    {}

    // parses out "nodes"
    void reply(msg const& m) override
    {
        flags |= flag_done;

        bdecode_node r = m.message.dict_find_dict("r");
        if (!r)
        {
#ifndef TORRENT_DISABLE_LOGGING
            if (get_observer())
            {
                get_observer()->nsw_log(nsw_logger_interface::node
                    , "[%p] missing response dict"
                    , static_cast<void*>(algorithm()));
            }
#endif
            return;
        }

        bdecode_node q_id = r.dict_find_string("q_id");

        node_id checked_id(q_id.string_ptr());

        algorithm()->get_node().m_table.heard_about(checked_id);
        // look for nodes
        // udp const protocol = algorithm()->get_node().get_protocol();
        // int const protocol_size = int(detail::address_size(protocol));
        // char const* nodes_key = algorithm()->get_node().get_protocol_nodes_key();
        // bdecode_node n = r.dict_find_string(nodes_key);
        // if (n)
        // {
        //  char const* nodes = n.string_ptr();
        //  char const* end = nodes + n.string_length();

        //  while (end - nodes >= 20 + protocol_size + 2)
        //  {
        //      dht::node_endpoint nep = dht::read_node_endpoint(protocol, nodes);
        //      algorithm()->get_node().m_table.heard_about(nep.id, nep.ep);
        //  }
        // }
    }
};

void node::tick()
{
    node_entry const* ne = m_table.next_for_refresh();
    if (ne == nullptr) return;

    // this shouldn't happen
    TORRENT_ASSERT(m_id != ne->id);
    if (ne->id == m_id) return;

    send_ping(ne->endpoint, ne->id);
}

void node::send_ping(udp::endpoint const& ep
                    , node_id const& id)
{
    TORRENT_ASSERT(id != m_id);

    // create a dummy traversal_algorithm
    auto const algo = std::make_shared<traversal_algorithm>(*this, m_id, m_table.get_descr());
    auto o = m_rpc.allocate_observer<ping_observer>(algo, ep, id);
    if (!o) return;

    entry e;
    e["y"] = "q";
    entry& a = e["a"];

    // just ping it.
    e["q"] = "ping";
    a["q_id"] = m_id.to_string();
    a["r_id"] = id.to_string();
    m_counters.inc_stats_counter(counters::nsw_ping_out);
    std::string fake;
    m_rpc.invoke(e, ep, o, fake);
}

time_duration node::connection_timeout()
{
    time_duration d = m_rpc.tick();
    time_point now(aux::time_now());
    if (now - minutes(2) < m_last_tracker_tick) return d;
    m_last_tracker_tick = now;

    return d;
}

void node::lookup_friends(sha1_hash const& info_hash
                        , vector_t const& target
                        , entry& reply
                        , address const& requester) const
{
    if (m_observer)
        m_observer->get_friends(info_hash, term_vector::vectorToString(target));
}

entry write_nodes_entry(std::vector<node_entry> const& closest_nodes
                                                , vector_t const& requested_vec
                                                , entry& e
                                                , int lvl
                                                , int lay)
{
    entry::list_type& pe = e.list();
    for (auto const& n : closest_nodes)
    {
        entry out;
        std::back_insert_iterator<std::string> out_itr(out.string());
        std::copy(n.id.begin(), n.id.end(), out_itr);

        double similarity = term_vector::getVecSimilarity(n.term_vector, requested_vec);

        std::copy((char*)&similarity, (char*)&similarity+sizeof(double), out_itr);
        std::copy((char*)&lvl, (char*)&lvl+sizeof(int), out_itr);
        std::copy((char*)&lay, (char*)&lay+sizeof(int), out_itr);

        detail::write_endpoint(udp::endpoint(n.addr(), std::uint16_t(n.port())), out_itr);

        pe.push_back(out);
    }
    return pe;
}

// build response
void node::incoming_request(msg const& m, entry& e)
{
    bool is_bootstrap_req = false;
    e = entry(entry::dictionary_t);
    e["y"] = "r";
    e["t"] = m.message.dict_find_string_value("t").to_string();

    key_desc_t const top_desc[] = {
        {"q", bdecode_node::string_t, 0, 0},
        {"a", bdecode_node::dict_t, 0, key_desc_t::parse_children},
        {"q_id", bdecode_node::string_t, 20, 0},
        {"q_lvl", bdecode_node::int_t, 0, 0},
        {"q_lay", bdecode_node::int_t, 0, 0},
        {"r_id", bdecode_node::string_t, 20, key_desc_t::last_child},
    };

    bdecode_node top_level[6];
    char error_string[200];

    if (!verify_message(m.message, top_desc, top_level, error_string))
    {
        incoming_error(e, error_string);
        return;
    }

    e["ip"] = endpoint_to_bytes(m.addr);

    bdecode_node arg_ent = top_level[1];
    node_id id(top_level[2].string_ptr());
    node_id requested_id(top_level[5].string_ptr());
    int q_level = top_level[3].int_value();
    int q_layout = top_level[4].int_value();

    if(requested_id.is_all_zeros())
    {
        //bootstrap
        is_bootstrap_req = true;
    }

    if(!is_bootstrap_req && m_id != requested_id)
    {
        m_observer->nsw_log(nsw_logger_interface::node, "message not for us! \
                requested id: %s and our id %s"
                , aux::to_hex(requested_id).c_str()
                , aux::to_hex(m_id).c_str());
        incoming_error(e, "Bad id",203);
        return;
    }

    entry& reply = e["r"];
    m_rpc.add_our_id(reply); //add q_id

    reply["r_id"] = id.to_string();
    // mirror back the other node's external port
    reply["p"] = m.addr.port();
    reply["q_lvl"] = q_level;
    reply["q_lay"] = q_layout;

    string_view query = top_level[0].string_value();

    if (m_observer && m_observer->on_nsw_request(query, m, e))
        return;

    if (query == "ping")
    {
        m_counters.inc_stats_counter(counters::nsw_ping_in);
        // we already have 't' and 'id' in the response
        // no more left to add
    }
    else if (query == "get_friends")
    {
        key_desc_t const msg_desc[] = {
            {"term_vector", bdecode_node::dict_t, 0, key_desc_t::parse_children},
        };

        bdecode_node msg_keys[1];
        if (!verify_message(arg_ent, msg_desc, msg_keys, error_string))
        {
//          m_counters.inc_stats_counter(counters::nsw_invalid_get_friends);
            incoming_error(e, error_string);
            return;
        }

        bdecode_node term_vector = msg_keys[0];

        write_nodes_entries(term_vector, reply, q_level, q_layout);

        // If our storage is full we want to withhold the write token so that
        // announces will spill over to our neighbors. This widens the
        // perimeter of nodes which store peers for this torrent
        reply["token"] = generate_token(m.addr, id);

#ifndef TORRENT_DISABLE_LOGGING
        if (reply.find_key("values") && m_observer)
        {
            m_observer->nsw_log(nsw_logger_interface::node, "values: %d"
                , int(reply["values"].list().size()));
        }
#endif
    }
    else if (query == "add_friend")
    {
        key_desc_t const msg_desc[] = {
            {"token", bdecode_node::string_t, 0, 0},
            {"description", bdecode_node::dict_t, 0, key_desc_t::parse_children},
        };

        bdecode_node add_friend_attributes[2];

        if (!verify_message(arg_ent, msg_desc, add_friend_attributes, error_string))
        {
            incoming_error(e, error_string);
            return;
        }


        if (!verify_token(add_friend_attributes[0].string_value()
            , sha1_hash(top_level[2].string_ptr()), m.addr))
        {
            incoming_error(e, "invalid token");
            return;
        }

        bdecode_node received_term_vector = add_friend_attributes[1];
        vector_t friend_descr;

        term_vector::bencodeToVector(received_term_vector,friend_descr);

        entry& term_vec = reply["description"];
        term_vector::vectorToEntry(m_table.get_descr(),term_vec);

        // the token was correct. That means this
        // node is not spoofing its address. So, let
        // the table get a chance to add it.
        m_table.add_friend(node_entry(id, m.addr, friend_descr
                                                        , ~0
                                                        , true)
                            , q_level
                            , q_layout);

        //m_table.node_seen(id, m.addr, 0xffff);
    }
    else if (query == "search_query")
    {
        key_desc_t const msg_desc[] = {
            {"token", bdecode_node::string_t, 0, 0},
        };

        bdecode_node search_query_attributes[1];

        if (!verify_message(arg_ent, msg_desc, search_query_attributes, error_string))
        {
            incoming_error(e, error_string);
            return;
        }


        if (!verify_token(search_query_attributes[0].string_value()
            , sha1_hash(top_level[2].string_ptr()), m.addr))
        {
            incoming_error(e, "invalid token");
            return;
        }

        reply["description"] = m_description;
        //term_vector::vectorToEntry(m_table.get_descr(),term_vec);
    }
    else
    {
        incoming_error(e, "unknown message");
        return;
    }
}


void node::write_nodes_entries(bdecode_node const& want, entry& r, int level, int layout)
{

    if (want.type() != bdecode_node::dict_t)
    {
        return;
    }
    if (!m_table.verifyLayoutInLevel(level, layout)) return;

    // if there is a wants entry then we may need to reach into
    // another node's routing table to get nodes of the requested type
    // we use a map maintained by the owning dht_tracker to find the
    // node associated with each string in the want list, which may
    // include this node
    vector_t got_vector;
    got_vector.reserve(want.dict_size());

    term_vector::bencodeToVector(want,got_vector);

    std::vector<node_entry> closest_friends;

    m_table.find_node(got_vector, closest_friends, m_table.neighbourhood_size(), level);

    double similarity_with_me = term_vector::getVecSimilarity(m_table.get_descr(), got_vector);
    double best_friend_similarity = (closest_friends.size() >0) ?
                            term_vector::getVecSimilarity(closest_friends.front().term_vector, got_vector):
                            0.0;

    if (best_friend_similarity < similarity_with_me)
    {
        char buf[sizeof(double)+1] = {0};
        std::memcpy(buf, &similarity_with_me, sizeof(double));
        r["me"] = buf;
    }

    entry& friends_vec = r["friends"];
    write_nodes_entry(closest_friends, got_vector, friends_vec, level, layout);
}

node::protocol_descriptor const& node::map_protocol_to_descriptor(udp protocol)
{
    static std::array<protocol_descriptor, 2> descriptors =
    {{
        {udp::v4(), "n4", "nodes"},
        {udp::v6(), "n6", "nodes6"}
    }};

    for (auto const& d : descriptors)
    {
        if (d.protocol == protocol)
            return d;
    }

    TORRENT_ASSERT_FAIL();
#ifndef BOOST_NO_EXCEPTIONS
    throw std::out_of_range("unknown protocol");
#else
    std::terminate();
#endif
}


void node::add_friend_engine(traversal_algorithm::callback_data_t const& v, nsw_lookup const& stat)
{
    // create a dummy traversal_algorithm
    auto algo = std::make_shared<traversal_algorithm>(*this, nid(), descr_vec());
    // store on the first k nodes
    for (auto const& p : v)
    {


        auto o = m_rpc.allocate_observer<add_friend_observer>(algo
                                                        , std::get<1>(p)
                                                        , std::get<0>(p)
                                                        , descr_vec()
                                                        );
        if (!o) return;
        entry e;
        e["y"] = "q";
        e["q"] = "add_friend";

        entry& a = e["a"];
        a["port"] = std::get<1>(p).port();
        a["token"] = std::get<2>(p);
        a["r_id"] = std::get<0>(p).to_string();
        a["q_lvl"] = std::get<5>(p);
        a["q_lay"] = std::get<6>(p);
        entry& term_vec = a["description"];
        term_vector::vectorToEntry(descr_vec(),term_vec);

        std::string fake;
        m_rpc.invoke(e, std::get<1>(p), o, fake);
    }
}

void node::nsw_query_engine(traversal_algorithm::callback_data_t const& v, nsw_lookup const& common_stat)
{
    m_last_query_stat = common_stat;

    // auto finalization_actions = [this](
    //      std::vector<std::pair<nsw::node_entry, std::string>> const&)
    //  {
    //      m_is_query_running = false;
    //      std::ofstream dump_file;
    //      dump_file.open("search_result.txt", std::ofstream::out | std::ofstream::app);
    //      dump_file << std::endl;
    //      dump_file << "\t\t### nodeID " << aux::to_hex(m_id).c_str() << " ###";
    //      dump_query_result(dump_file);
    //      dump_file.close();
    //  };

    auto algo = std::make_shared<traversal_algorithm>(*this, nid()
                                                    , descr_vec());
    m_search_routines = v.size();

    for (auto const& p : v)
    {

        auto o = m_rpc.allocate_observer<search_query_observer>(algo
                                                        , std::get<1>(p)
                                                        , std::get<0>(p)
                                                        , descr_vec());

        if (!o) return;
        entry e;
        e["y"] = "q";
        e["q"] = "search_query";

        entry& a = e["a"];
        a["port"] = std::get<1>(p).port();
        a["token"] = std::get<2>(p);
        a["r_id"] = std::get<0>(p).to_string();
        std::string fake;
        m_rpc.invoke(e, std::get<1>(p), o, fake);
    }
}

void add_friend_observer::reply(libtorrent::nsw::msg const& m)
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

    bdecode_node q_id = r.dict_find_string("q_id");

    int lvl = r.dict_find_int("q_lvl").int_value();
    int lay = r.dict_find_int("q_lay").int_value();

    node_id querying_id(q_id.string_ptr());

    bdecode_node received_term_vector = r.dict_find_dict("description");

    vector_t friend_descr;
    term_vector::bencodeToVector(received_term_vector,friend_descr);

    algorithm()->get_node().m_table.add_friend(node_entry(querying_id
                                                            , m.addr
                                                            , friend_descr
                                                            , ~0
                                                            , true)
                                               , lvl
                                               , lay);
    flags |= flag_done;
}

void search_query_observer::reply(libtorrent::nsw::msg const& m)
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

    bdecode_node q_id = r.dict_find_string("q_id");

    node_id querying_id(q_id.string_ptr());

    bdecode_node received_description = r.dict_find_string("description");

    std::string description_str(received_description.string_ptr(),received_description.string_length());

    vector_t description_vec;
    term_vector::makeTermVector(description_str,description_vec);
    algorithm()->get_node().handle_nsw_query_result(description_str
                        , term_vector::getVecSimilarity(algorithm()->get_node().get_stat().target,description_vec));
    //m_out_container.get()->push_back(description_str);
    //m_callback(description_str);
    flags |= flag_done;
}

void node::handle_nsw_query_result(std::string& result, double similarity_with_query)
{
    std::lock_guard<std::mutex> l(m_mutex);
    --m_search_routines;
    auto existed_result = m_query_results.find(similarity_with_query);

    if(existed_result == m_query_results.end())
        m_query_results.insert(std::make_pair(similarity_with_query,result));

    // for debugging purposes
    if(!m_search_routines)
    {
        m_is_query_running = false;
        std::ofstream dump_file;
        dump_file.open("search_result.txt", std::ofstream::out | std::ofstream::app);
        dump_file << std::endl;
        dump_file << "\t\t### nodeID " << aux::to_hex(m_id).c_str() << " ###\n";
        dump_query_result(dump_file);
        dump_file.close();
    }
}

void node::get_query_result(std::vector<std::string>& results_copy)
{
    if(!m_is_query_running)
    {
        std::for_each(m_query_results.begin()
                    , m_query_results.end()
                    , [&results_copy](std::pair<double, std::string> const& item)
                        {
                            results_copy.push_back("simil:" + std::to_string(item.first) + "| "+ item.second);
                        });
    }
}

void node::dump_query_result(std::ofstream& dump_file)
{
    if(!m_is_query_running)
    {
        std::for_each(m_query_results.begin()
                    , m_query_results.end()
                    , [&dump_file, this](std::pair<double, std::string> const& item)
                        {
                            std::stringstream ss;
                            ss << std::fixed << std::setprecision(15) << item.first;
                            dump_file << "simil:" + ss.str() + " | metrics: " +  std::to_string(m_last_query_stat.known_nodes) + " | " + item.second;
                            dump_file << std::endl;
                        });
    }
}

} }
