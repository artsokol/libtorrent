#ifndef NSW_MSG_HPP
#define NSW_MSG_HPP

#include "libtorrent/socket.hpp"
#include "libtorrent/span.hpp"

namespace libtorrent {

struct bdecode_node;

namespace nsw{


struct msg
{
	msg(bdecode_node const& m, udp::endpoint const& ep): message(m), addr(ep) {}

	bdecode_node const& message;

	udp::endpoint addr;


    //msg_tools

    //we concider numberis positive
    static int count_digits(int arg) {
        return std::snprintf(NULL, 0, "%d", arg);
    }
private:
	msg& operator=(msg const&);


};

struct key_desc_t
{
    char const* name;
    int type;
    int size;
    int flags;

    enum {
        optional = 1,
        parse_children = 2,
        last_child = 4,
        size_divisible = 8
    };
};

TORRENT_EXTRA_EXPORT bool verify_message_impl(bdecode_node const& msg, span<key_desc_t const> desc
    , span<bdecode_node> ret, span<char> error);

template <int Size>
bool verify_message(bdecode_node const& msg, key_desc_t const (&desc)[Size]
    , bdecode_node (&ret)[Size], span<char> error)
{
    return verify_message_impl(msg, desc, ret, error);
}

} }

#endif  // NSW_MSG_HPP
