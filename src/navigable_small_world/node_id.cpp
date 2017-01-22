#include <algorithm>

#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/navigable_small_world/node_entry.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/broadcast_socket.hpp"
#include "libtorrent/socket_io.hpp"
#include "libtorrent/random.hpp"
#include "libtorrent/hasher.hpp"
#include "libtorrent/crc32c.hpp"

namespace libtorrent {
namespace nsw
{

node_id distance(node_id const& n1, node_id const& n2)
{
	node_id ret;
	(void)n1;
	(void)n2;
	return ret;
}


bool compare_ref(node_id const& n1, node_id const& n2, node_id const& ref)
{
	(void)n1;
	(void)n2;
	(void)ref;
	return false;
}


int distance_exp(node_id const& n1, node_id const& n2)
{

	(void)n1;
	(void)n2;

	return 0;
}

node_id generate_id_impl(address const& ip_, boost::uint32_t r)
{
	node_id id;
	(void)ip_;
	(void)r;

	return id;
}

void make_id_secret(node_id& in)
{
	(void)in;
}

node_id generate_random_id()
{
	node_id id;
	return id;
}

node_id generate_secret_id()
{
	node_id id;
	return id;
}

bool verify_secret_id(node_id const& nid)
{
	(void)nid;
	return false;
}

bool verify_id(node_id const& nid, address const& source_ip)
{
	(void)nid;
	(void)source_ip;
	return false;
}

node_id generate_id(address const& ip)
{
	node_id id;
	(void)ip;
	return id;
}

bool matching_prefix(node_entry const& n, int mask, int prefix, int bucket_index)
{
	(void)n;
	(void)mask;
	(void)prefix;
	(void)bucket_index;
	return false;
}

node_id generate_prefix_mask(int bits)
{

	node_id mask(0);
	(void)bits;
	return mask;
}

} }
