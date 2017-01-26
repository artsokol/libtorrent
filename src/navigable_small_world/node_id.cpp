#include <algorithm>

#include "libtorrent/navigable_small_world/node_id.hpp"
#include "libtorrent/assert.hpp"
#include "libtorrent/broadcast_socket.hpp" // for is_local et.al
#include "libtorrent/random.hpp" // for random
#include "libtorrent/hasher.hpp" // for hasher
#include "libtorrent/crc32c.hpp" // for crc32c

namespace libtorrent {
namespace nsw
{

node_id generate_id_impl(address const& ip_, std::uint32_t r)
{
	std::uint8_t* ip = nullptr;

	static const std::uint8_t v4mask[] = { 0x03, 0x0f, 0x3f, 0xff };
#if TORRENT_USE_IPV6
	static const std::uint8_t v6mask[] = { 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
#endif
	std::uint8_t const* mask = nullptr;
	int num_octets = 0;

	address_v4::bytes_type b4;
#if TORRENT_USE_IPV6
	address_v6::bytes_type b6;
	if (ip_.is_v6())
	{
		b6 = ip_.to_v6().to_bytes();
		ip = b6.data();
		num_octets = 8;
		mask = v6mask;
	}
	else
#endif
	{
		b4 = ip_.to_v4().to_bytes();
		ip = b4.data();
		num_octets = 4;
		mask = v4mask;
	}

	for (int i = 0; i < num_octets; ++i)
		ip[i] &= mask[i];

	ip[0] |= (r & 0x7) << 5;

	// this is the crc32c (Castagnoli) polynomial
	std::uint32_t c;
	if (num_octets == 4)
	{
		c = crc32c_32(*reinterpret_cast<std::uint32_t*>(ip));
	}
	else
	{
		TORRENT_ASSERT(num_octets == 8);
		c = crc32c(reinterpret_cast<std::uint64_t*>(ip), 1);
	}
	node_id id;

	id[0] = (c >> 24) & 0xff;
	id[1] = (c >> 16) & 0xff;
	id[2] = (((c >> 8) & 0xf8) | random(0x7)) & 0xff;

	for (std::size_t i = 3; i < 19; ++i) id[i] = random(0xff) & 0xff;
	id[19] = r & 0xff;

	return id;
}

inline node_id generate_id(address const& ip)
{
	return generate_id_impl(ip, random(0xffffffff));
}

node_id generate_random_id()
{
	char r[20];
	aux::random_bytes(r);
	return hasher(r, 20).final();
}

//See the header for commenting explanation
/*
void make_id_secret(node_id& in)
{
	(void)in;
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
*/

bool verify_id(node_id const& nid, address const& source_ip)
{
	// no need to verify local IPs, they would be incorrect anyway
	if (is_local(source_ip)) return true;

	node_id h = generate_id_impl(source_ip, nid[19]);
	return nid[0] == h[0] && nid[1] == h[1] && (nid[2] & 0xf8) == (h[2] & 0xf8);
}

bool matching_prefix(node_id const& nid, int mask, int prefix, int offset)
{
	node_id id = nid;
	id <<= offset;
	return (id[0] & mask) == prefix;
}

node_id generate_prefix_mask(int bits)
{
	TORRENT_ASSERT(bits >= 0);
	TORRENT_ASSERT(bits <= 160);
	node_id mask;
	std::size_t b = 0;
	for (; int(b) < bits - 7; b += 8) mask[b / 8] |= 0xff;
	if (bits < 160) mask[b / 8] |= (0xff << (8 - (bits & 7))) & 0xff;
	return mask;
}

} }
