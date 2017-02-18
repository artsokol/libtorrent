#include "libtorrent/navigable_small_world/nsw_storage.hpp"


#include <tuple>
#include <algorithm>
#include <utility>
#include <map>
#include <set>
#include <string>

#include <libtorrent/socket_io.hpp>
#include <libtorrent/aux_/time.hpp>
#include <libtorrent/config.hpp>
#include <libtorrent/bloom_filter.hpp>
#include <libtorrent/session_settings.hpp>
#include <libtorrent/random.hpp>

namespace libtorrent {
namespace nsw {
namespace
{
	struct peer_entry
	{
		time_point added;
		tcp::endpoint addr;
		bool seed;
	};

	// bool operator<(peer_entry const& lhs, peer_entry const& rhs)
	// {
	// 	return lhs.addr.address() == rhs.addr.address()
	// 		? lhs.addr.port() < rhs.addr.port()
	// 		: lhs.addr.address() < rhs.addr.address();
	// }

	struct torrent_entry
	{
		std::string name;
		std::vector<peer_entry> peers4;
		std::vector<peer_entry> peers6;
	};

	//  enum { announce_interval = 30 };

	struct nsw_immutable_item
	{
		std::unique_ptr<char[]> value;
		bloom_filter<128> ips;
		time_point last_seen;
		int num_announcers = 0;
		int size = 0;
	};

	struct nsw_mutable_item : nsw_immutable_item
	{
		signature sig;
		sequence_number seq;
		public_key key;
		std::string salt;
	};

	void set_value(nsw_immutable_item& item, span<char const> buf)
	{
	}

	void touch_item(nsw_immutable_item& f, address const& addr)
	{
	}


	// struct immutable_item_comparator
	// {
	// 	explicit immutable_item_comparator(std::vector<node_id> const& node_ids) : m_node_ids(node_ids) {}
	// 	immutable_item_comparator(immutable_item_comparator const&) = default;

	// 	template <typename Item>
	// 	bool operator()(std::pair<node_id const, Item> const& lhs
	// 		, std::pair<node_id const, Item> const& rhs) const
	// 	{
	// 		int const l_distance = min_distance_exp(lhs.first, m_node_ids);
	// 		int const r_distance = min_distance_exp(rhs.first, m_node_ids);

	// 		return lhs.second.num_announcers / 5 - l_distance < rhs.second.num_announcers / 5 - r_distance;
	// 	}

	// private:

	// 	immutable_item_comparator& operator=(immutable_item_comparator const&);

	// 	std::vector<node_id> const& m_node_ids;
	// };

	// template<class Item>
	// typename std::map<node_id, Item>::const_iterator pick_least_important_item(
	// 	std::vector<node_id> const& node_ids, std::map<node_id, Item> const& table)
	// {
	// 	return std::min_element(table.begin(), table.end()
	// 		, immutable_item_comparator(node_ids));
	// }

	class nsw_default_storage final : public nsw_storage_interface, boost::noncopyable
	{
	public:

		explicit nsw_default_storage(nsw_settings const& settings)
			: m_settings(settings)
		{
		}

		~nsw_default_storage() override = default;

#ifndef TORRENT_NO_DEPRECATE
		size_t num_torrents() const override { return m_map.size(); }
		size_t num_peers() const override
		{
		}
#endif
		void update_node_ids(std::vector<node_id> const& ids) override
		{
		}

		bool get_peers(sha1_hash const& info_hash
			, bool const noseed, bool const scrape, address const& requester
			, entry& peers) const override
		{
			return false;
		}

		// void announce_peer(sha1_hash const& info_hash
		// 	, tcp::endpoint const& endp
		// 	, string_view name, bool const seed) override
		// {
		// }

		// bool get_immutable_item(sha1_hash const& target
		// 	, entry& item) const override
		// {
		// 	return true;
		// }

		// void put_immutable_item(sha1_hash const& target
		// 	, span<char const> buf
		// 	, address const& addr) override
		// {
		// }

		// bool get_mutable_item_seq(sha1_hash const& target
		// 	, sequence_number& seq) const override
		// {

		// 	return true;
		// }

		// bool get_mutable_item(sha1_hash const& target
		// 	, sequence_number const seq, bool const force_fill
		// 	, entry& item) const override
		// {
		// 	return true;
		// }

		// void put_mutable_item(sha1_hash const& target
		// 	, span<char const> buf
		// 	, signature const& sig
		// 	, sequence_number const seq
		// 	, public_key const& pk
		// 	, span<char const> salt
		// 	, address const& addr) override
		// {
		// }

		void tick() override
		{
		}

		nsw_storage_counters counters() const override
		{
			return m_counters;
		}

	private:
		nsw_settings const& m_settings;
		nsw_storage_counters m_counters;

		std::vector<node_id> m_node_ids;
		std::map<node_id, torrent_entry> m_map;
		// std::map<node_id, nsw_immutable_item> m_immutable_table;
		// std::map<node_id, nsw_mutable_item> m_mutable_table;

		void purge_peers(std::vector<peer_entry>& peers)
		{
		}
	};
}

void nsw_storage_counters::reset()
{
	torrents = 0;
	peers = 0;
	immutable_data = 0;
	mutable_data = 0;
}

std::unique_ptr<nsw_storage_interface> nsw_default_storage_constructor(
	nsw_settings const& settings)
{
	return std::unique_ptr<nsw_default_storage>(new nsw_default_storage(settings));
}

} } // namespace libtorrent::nsw
