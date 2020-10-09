// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

class BitUnpacker
{
public:
	BitUnpacker(const std::vector<std::uint8_t>& buffer) : buffer_{buffer}, current_bit_index_{0}
	{
	}

	std::size_t BitsRemaining() const
	{
		return (8 * buffer_.size()) - current_bit_index_;
	}

	std::uint64_t ReadSymbol(std::size_t num_bits)
	{
		assert(num_bits <= BitsRemaining());
		assert(num_bits <= 64);
		auto symbol = PeekSymbol(num_bits);
		SkipBits(num_bits);
		return symbol;
	}

	void SkipBits(std::size_t num_bits)
	{
		current_bit_index_ += num_bits;
	}

	std::uint64_t PeekSymbol(std::size_t num_bits) const
	{
		assert(num_bits <= BitsRemaining());
		assert(num_bits <= 64);

		std::uint64_t symbol = 0;
		std::size_t num_bits_read = 0;

		while (num_bits_read < num_bits)
		{
			auto next_bit_index = current_bit_index_ + num_bits_read;
			auto current_byte_index = next_bit_index / 8u;
			auto bits_read_in_current_byte = next_bit_index % 8u;
			auto bits_left_in_current_byte = 8u - bits_read_in_current_byte;
			auto bits_left_to_read = num_bits - num_bits_read;

			auto bits_to_read_from_current_byte = std::min(bits_left_in_current_byte, bits_left_to_read);

			std::uint64_t symbol_from_this_byte =
				(buffer_[current_byte_index] >> bits_read_in_current_byte) & ((1ull << bits_to_read_from_current_byte) - 1ull);
			symbol |= symbol_from_this_byte << num_bits_read;

			num_bits_read += bits_to_read_from_current_byte;
		}

		assert((symbol & ~((1ull << num_bits) - 1ull)) == 0ull);

		return symbol;
	}

private:
	const std::vector<std::uint8_t>& buffer_;
	std::size_t current_bit_index_;
};
