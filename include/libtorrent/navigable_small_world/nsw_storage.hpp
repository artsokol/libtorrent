
#ifndef NSW_STORAGE_HPP
#define NSW_STORAGE_HPP

#include <functional>

#include <libtorrent/navigable_small_world/node_id.hpp>
#include <libtorrent/navigable_small_world/types.hpp>

#include <libtorrent/socket.hpp>
#include <libtorrent/address.hpp>
#include <libtorrent/span.hpp>
#include <libtorrent/string_view.hpp>

namespace libtorrent
{
	struct nsw_settings;
	class entry;
}

namespace libtorrent { namespace nsw{

	struct TORRENT_EXPORT nsw_storage_counters
	{
		std::int32_t torrents;
		std::int32_t peers;
		std::int32_t immutable_data;
		std::int32_t mutable_data;
		void reset();
	};

	struct TORRENT_EXPORT nsw_storage_interface
	{
#ifndef TORRENT_NO_DEPRECATE

		virtual size_t num_torrents() const = 0;

		virtual size_t num_peers() const = 0;
#endif

		virtual void update_node_ids(std::vector<node_id> const& ids) = 0;

		virtual bool get_peers(sha1_hash const& info_hash
			, bool noseed, bool scrape, address const& requester
			, entry& peers) const = 0;

		// virtual void announce_peer(sha1_hash const& info_hash
		// 	, tcp::endpoint const& endp
		// 	, string_view name, bool seed) = 0;

		// virtual bool get_immutable_item(sha1_hash const& target
		// 	, entry& item) const = 0;

		// virtual void put_immutable_item(sha1_hash const& target
		// 	, span<char const> buf
		// 	, address const& addr) = 0;

		// virtual bool get_mutable_item_seq(sha1_hash const& target
		// 	, sequence_number& seq) const = 0;

		// virtual bool get_mutable_item(sha1_hash const& target
		// 	, sequence_number seq, bool force_fill
		// 	, entry& item) const = 0;

		// virtual void put_mutable_item(sha1_hash const& target
		// 	, span<char const> buf
		// 	, signature const& sig
		// 	, sequence_number seq
		// 	, public_key const& pk
		// 	, span<char const> salt
		// 	, address const& addr) = 0;

		virtual void tick() = 0;

		virtual nsw_storage_counters counters() const = 0;

		virtual ~nsw_storage_interface() {}
	};

	using nsw_storage_constructor_type
		= std::function<std::unique_ptr<nsw_storage_interface>(nsw_settings const& settings)>;

	TORRENT_EXPORT std::unique_ptr<nsw_storage_interface>
		nsw_default_storage_constructor(nsw_settings const& settings);

} }

#endif // NSW_STORAGE_HPP
