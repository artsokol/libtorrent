#ifndef NSW_NODE_ID_HPP
#define NSW_NODE_ID_HPP

#include <vector>
#include <cstdint>

#include <libtorrent/config.hpp>
#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/address.hpp>


namespace libtorrent { 
namespace nsw 
{

using node_id = libtorrent::sha1_hash;


TORRENT_EXTRA_EXPORT node_id distance(node_id const& n1, node_id const& n2);

TORRENT_EXTRA_EXPORT bool compare_ref(node_id const& n1, node_id const& n2, node_id const& ref);


TORRENT_EXTRA_EXPORT int distance_exp(node_id const& n1, node_id const& n2);
TORRENT_EXTRA_EXPORT int min_distance_exp(node_id const& n1, std::vector<node_id> const& ids);

TORRENT_EXTRA_EXPORT node_id generate_id(address const& external_ip);
TORRENT_EXTRA_EXPORT node_id generate_random_id();
TORRENT_EXTRA_EXPORT void make_id_secret(node_id& in);
TORRENT_EXTRA_EXPORT node_id generate_secret_id();
TORRENT_EXTRA_EXPORT bool verify_secret_id(node_id const& nid);
TORRENT_EXTRA_EXPORT node_id generate_id_impl(address const& ip_, std::uint32_t r);

TORRENT_EXTRA_EXPORT bool verify_id(node_id const& nid, address const& source_ip);
TORRENT_EXTRA_EXPORT bool matching_prefix(node_id const& nid, int mask, int prefix, int offset);
TORRENT_EXTRA_EXPORT node_id generate_prefix_mask(int bits);

} }

#endif // NSW_NODE_ID_HPP

