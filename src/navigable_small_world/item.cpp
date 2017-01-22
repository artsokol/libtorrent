#include <libtorrent/hasher.hpp>
#include <libtorrent/navigable_small_world/item.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/ed25519.hpp>

#ifdef TORRENT_DEBUG
#include "libtorrent/bdecode.hpp"
#endif

namespace libtorrent { namespace nsw
{

namespace
{
	enum { canonical_length = 1200 };
	int canonical_string(std::pair<char const*, int> v, boost::uint64_t seq
		, std::pair<char const*, int> salt, char out[canonical_length])
	{
		(void)v;
		(void)seq;
		(void)salt;
		(void)out;
		return 0;
	}
}

sha1_hash item_target_id(std::pair<char const*, int> v)
{
	(void)v;
	sha1_hash  tmp;
	return tmp;
}

sha1_hash item_target_id(std::pair<char const*, int> salt
	, char const* pk)
{
	(void)salt;
	(void)pk;
	sha1_hash  tmp;
	return tmp;
}

bool verify_mutable_item(
	std::pair<char const*, int> v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq
	, char const* pk
	, char const* sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
	return false;
}

void sign_mutable_item(
	std::pair<char const*, int> v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq
	, char const* pk
	, char const* sk
	, char* sig)
{

	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sk;
	(void)sig;
}

item::item(char const* pk, std::string const& salt)
	: m_salt(salt)
	, m_seq(0)
	, m_mutable(true)
{
	memcpy(m_pk.data(), pk, item_pk_len);
}

item::item(entry const& v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq, char const* pk, char const* sk)
{
	assign(v, salt, seq, pk, sk);
}

void item::assign(entry const& v, std::pair<char const*, int> salt
	, boost::uint64_t seq, char const* pk, char const* sk)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sk;
}

bool item::assign(bdecode_node const& v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq, char const* pk, char const* sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
	return true;
}

void item::assign(entry const& v, std::string salt, boost::uint64_t seq
	, char const* pk, char const* sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
}

} }
