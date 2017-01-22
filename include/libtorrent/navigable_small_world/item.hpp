#ifndef NSW_ITEM_HPP
#define NSW_ITEM_HPP

#include <libtorrent/sha1_hash.hpp>
#include <libtorrent/bdecode.hpp>
#include <libtorrent/entry.hpp>
#include <vector>
#include <exception>
#include <boost/array.hpp>

namespace libtorrent {
namespace nsw
{

sha1_hash TORRENT_EXTRA_EXPORT item_target_id(
	std::pair<char const*, int> v);

sha1_hash TORRENT_EXTRA_EXPORT item_target_id(std::pair<char const*, int> salt
	, char const* pk);

bool TORRENT_EXTRA_EXPORT verify_mutable_item(
	std::pair<char const*, int> v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq
	, char const* pk
	, char const* sig);

void TORRENT_EXPORT sign_mutable_item(
	std::pair<char const*, int> v
	, std::pair<char const*, int> salt
	, boost::uint64_t seq
	, char const* pk
	, char const* sk
	, char* sig);

enum
{
	item_pk_len = 32,
	item_sk_len = 64,
	item_sig_len = 64
};

class TORRENT_EXTRA_EXPORT item
{
public:
	item() : m_seq(0), m_mutable(false)  {}
	item(char const* pk, std::string const& salt);
	item(entry const& v) { assign(v); }
	item(entry const& v
		, std::pair<char const*, int> salt
		, boost::uint64_t seq, char const* pk, char const* sk);
	item(bdecode_node const& v) { assign(v); }

	void assign(entry const& v)
	{
		assign(v, std::pair<char const*, int>(static_cast<char const*>(NULL)
			, 0), 0, NULL, NULL);
	}
	void assign(entry const& v, std::pair<char const*, int> salt
		, boost::uint64_t seq, char const* pk, char const* sk);
	void assign(bdecode_node const& v)
	{
		assign(v, std::pair<char const*, int>(static_cast<char const*>(NULL)
			, 0), 0, NULL, NULL);
	}
	bool assign(bdecode_node const& v, std::pair<char const*, int> salt
		, boost::uint64_t seq, char const* pk, char const* sig);
	void assign(entry const& v, std::string salt, boost::uint64_t seq
		, char const* pk, char const* sig);

	void clear() { m_value = entry(); }
	bool empty() const { return m_value.type() == entry::undefined_t; }

	bool is_mutable() const { return m_mutable; }

	entry const& value() const { return m_value; }
	boost::array<char, item_pk_len> const& pk() const
	{ return m_pk; }
	boost::array<char, item_sig_len> const& sig() const
	{ return m_sig; }
	boost::uint64_t seq() const { return m_seq; }
	std::string const& salt() const { return m_salt; }

private:
	entry m_value;
	std::string m_salt;
	boost::array<char, item_pk_len> m_pk;
	boost::array<char, item_sig_len> m_sig;
	boost::uint64_t m_seq;
	bool m_mutable;
};

} }

#endif // NSW_ITEM_HPP
