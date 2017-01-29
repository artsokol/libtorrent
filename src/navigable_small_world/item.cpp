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

	int canonical_string(span<char const> v
		, sequence_number const seq
		, span<char const> salt
		, span<char> out)
	{
		(void)v;
		(void)seq;
		(void)salt;
		(void)out;
		return 0;
	}
}
sha1_hash item_target_id(span<char const> v)
{
	(void)v;
	sha1_hash  tmp;
	return tmp;
}

sha1_hash item_target_id(span<char const> salt
	, public_key const& pk)
{
	(void)salt;
	(void)pk;
	sha1_hash  tmp;
	return tmp;
}

bool verify_mutable_item(
	span<char const> v
	, span<char const> salt
	, sequence_number const seq
	, public_key const& pk
	, signature const& sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
	return false;
}

signature sign_mutable_item(
	span<char const> v
	, span<char const> salt
	, sequence_number const seq
	, public_key const& pk
	, secret_key const& sk)
{

	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sk;
}

item::item(public_key const& pk, span<char const> salt)
	: m_salt(salt.data(), salt.size())
	, m_pk(pk)
	, m_seq(0)
	, m_mutable(true)
{}


item::item(entry v)
	: m_value(std::move(v))
	, m_seq(0)
	, m_mutable(false)
{}

item::item(bdecode_node const& v)
	: m_seq(0)
	, m_mutable(false)
{
	// TODO: implement ctor for entry from bdecode_node?
	m_value = v;
}

item::item(entry v, span<char const> salt
	, sequence_number const seq, public_key const& pk, secret_key const& sk)
{}


void item::assign(entry v)
{
	(void)v;
}

void item::assign(bdecode_node const& v)
{
	(void)v;
}

bool item::assign(bdecode_node const& v, span<char const> salt
	, sequence_number const seq, public_key const& pk, signature const& sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
	return false;
}

void item::assign(entry v, span<char const> salt
	, sequence_number const seq
	, public_key const& pk, signature const& sig)
{
	(void)v;
	(void)salt;
	(void)seq;
	(void)pk;
	(void)sig;
}

} }
