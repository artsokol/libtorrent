#include "libtorrent/navigable_small_world/msg.hpp"
#include "libtorrent/bdecode.hpp"
#include "libtorrent/entry.hpp"

namespace libtorrent { 
namespace nsw{

namespace nsw_detail {

bool verify_message(bdecode_node const& message, key_desc_t const desc[]
	, bdecode_node ret[], int size, char* error, int error_size)
{
	(void)message;
	(void*)desc;
	(void*)ret;
	(void)size;
	(void*)error;
	(void)error_size;
	return true;
}

}

void incoming_error(entry& e, char const* msg, int error_code)
{
	(void)e;
	(void)msg;
	(void)error_code;
}

} }
