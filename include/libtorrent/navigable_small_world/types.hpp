#ifndef NSW_TYPES_HPP
#define NSW_TYPES_HPP

#include <cstdint>

namespace libtorrent { namespace nsw
{
	struct public_key
	{
		public_key() = default;
		explicit public_key(char const* b)
		{ std::copy(b, b + len, bytes.begin()); }
		bool operator==(public_key const& rhs) const
		{ return bytes == rhs.bytes; }
		constexpr static int len = 32;
		std::array<char, len> bytes;
	};

	struct secret_key
	{
		secret_key() = default;
		explicit secret_key(char const* b)
		{ std::copy(b, b + len, bytes.begin()); }
		bool operator==(secret_key const& rhs) const
		{ return bytes == rhs.bytes; }
		constexpr static int len = 64;
		std::array<char, len> bytes;
	};

	struct signature
	{
		signature() = default;
		explicit signature(char const* b)
		{ std::copy(b, b + len, bytes.begin()); }
		bool operator==(signature const& rhs) const
		{ return bytes == rhs.bytes; }
		constexpr static int len = 64;
		std::array<char, len> bytes;
	};

	struct sequence_number
	{
		sequence_number() : value(0) {}
		explicit sequence_number(std::int64_t v) : value(v) {}
		sequence_number(sequence_number const& sqn) = default;
		bool operator<(sequence_number rhs) const
		{ return value < rhs.value; }
		bool operator>(sequence_number rhs) const
		{ return value > rhs.value; }
		sequence_number& operator=(sequence_number rhs)
		{ value = rhs.value; return *this; }
		bool operator<=(sequence_number rhs) const
		{ return value <= rhs.value; }
		bool operator==(sequence_number const& rhs) const
		{ return value == rhs.value; }
		sequence_number& operator++()
		{ ++value; return *this; }
		std::int64_t value;
	};

}}

#endif // NSW_TYPES_HPP
