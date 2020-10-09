// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#pragma once

#include "BitPacker.h"
#include "BitUnpacker.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <deque>
#include <iostream>
#include <vector>

class PrefixEncoding
{
public:
	PrefixEncoding(std::size_t num_symbols, const std::vector<std::uint64_t>& symbol_frequencies)
		: num_symbols_{num_symbols}, current_symbol_frequencies(num_symbols_, 0)
	{
		assert(symbol_frequencies.size() == num_symbols);

		sorted_symbols_.resize(num_symbols_);
		cumulative_frequencies_of_sorted_symbols_.resize(num_symbols_);
		symbol_num_bits.resize(num_symbols_);
		symbol_encodings.resize(num_symbols_);

		for (std::size_t symbol = 0; symbol < num_symbols_; symbol++)
		{
			auto frequency = 1 + symbol_frequencies[symbol];
			sorted_symbols_[symbol] = (Symbol{symbol, frequency});
		}
		std::stable_sort(sorted_symbols_.begin(), sorted_symbols_.end(),
			[](const Symbol& lhs, const Symbol& rhs) { return lhs.frequency > rhs.frequency; });
		std::size_t total_frequency = 0;
		cumulative_frequencies_of_sorted_symbols_.resize(num_symbols);
		for (auto i = 0; i < num_symbols_; i++)
		{
			total_frequency += sorted_symbols_[i].frequency;
			cumulative_frequencies_of_sorted_symbols_[i] = total_frequency;
		}

		FindBestTiers();
	}

	inline void EncodeSymbol(std::uint64_t symbol, BitPacker& bits, bool track_symbols)
	{
		assert(symbol < num_symbols_);
		auto num_bits = symbol_num_bits[symbol];
		bits.WriteBack(symbol_encodings[symbol], num_bits);

		if (track_symbols)
		{
			current_symbol_frequencies[symbol]++;
		}
	}

	inline std::uint64_t DecodeSymbol(BitUnpacker& bits, bool track_symbols)
	{
		for (auto tier_index = 0ul; tier_index < kNumTiers; tier_index++)
		{
			auto prefix = bits.PeekSymbol(tier_bit_prefix_sizes_[tier_index]);

			if (tier_bit_prefixes_[tier_index] == prefix)
			{
				// This is the tier.
				bits.SkipBits(tier_bit_prefix_sizes_[tier_index]);
				auto symbol_index_in_tier = bits.ReadSymbol(tier_bit_widths_[tier_index]);
				auto remapped_symbol = symbol_index_in_tier + tier_start_symbol_index_[tier_index];
				auto symbol = sorted_symbols_[remapped_symbol].symbol;

				if (track_symbols)
				{
					current_symbol_frequencies[symbol]++;
				}

				return symbol;
			}
		}

		assert(false);
		return 0;
	}

	inline std::size_t NumBitsForSymbol(std::uint64_t symbol) const
	{
		return symbol_num_bits[symbol];
	}

	inline void EncodeUint64Varint(std::uint64_t value, BitPacker& bits, bool track_symbols)
	{
		constexpr auto kLowerChunkMask = 0b1111111u;
		while (true)
		{
			std::uint8_t chunk = value & kLowerChunkMask;
			if ((value & ~(kLowerChunkMask)) != 0)
			{
				// There are more chunks to come.
				chunk |= 0b10000000u;
				EncodeSymbol(chunk, bits, track_symbols);
			}
			else
			{
				EncodeSymbol(chunk, bits, track_symbols);
				break;
			}
			value >>= 7u;
		}
	}

	inline std::uint64_t DecodeUint64Varint(BitUnpacker& bits, bool track_symbols)
	{
		std::uint64_t value = 0;
		std::size_t chunk_index = 0;
		while (true)
		{
			std::uint8_t chunk = DecodeSymbol(bits, track_symbols);
			value |= (static_cast<std::uint64_t>(chunk) & ((1ul << 7ul) - 1ul)) << (7ul * chunk_index);
			chunk_index++;
			if (chunk >> 7u == 0u)
			{
				// No more chunks.
				break;
			}
		}
		assert(chunk_index <= 10);
		return value;
	}

	const std::vector<std::uint64_t>& GetSymbolFrequencies() const
	{
		return current_symbol_frequencies;
	}

private:
	// This is ExpectedBitsPerSymbol * total_frequencies
	std::size_t CalculateFactorOfExpectedBitsPerSymbol()
	{
		std::size_t expected_bits_factor = 0;
		std::size_t tier_first_symbol_index = 0ull;
		for (std::size_t tier_index = 0ull; tier_index < kNumTiers; tier_index++)
		{
			std::size_t num_symbols_in_tier = 1ull << tier_bit_widths_[tier_index];
			// Inclusive
			auto first_symbol_index = tier_first_symbol_index;
			// Non inclusive index
			auto last_symbol_index = std::min(first_symbol_index + num_symbols_in_tier, num_symbols_);
			auto sum_of_frequencies_before_tier = 0;
			auto sum_of_frequencies_up_to_end_of_tier = 0;
			if (first_symbol_index >= 1)
			{
				sum_of_frequencies_before_tier = cumulative_frequencies_of_sorted_symbols_[first_symbol_index - 1];
			}
			if (last_symbol_index >= 1)
			{
				sum_of_frequencies_up_to_end_of_tier = cumulative_frequencies_of_sorted_symbols_[last_symbol_index - 1];
			}
			auto sum_of_frequencies_in_tier = sum_of_frequencies_up_to_end_of_tier - sum_of_frequencies_before_tier;
			assert(sum_of_frequencies_in_tier >= 0);
			auto size_of_symbol_in_tier = tier_bit_prefix_sizes_[tier_index] + tier_bit_widths_[tier_index];

			expected_bits_factor += sum_of_frequencies_in_tier * size_of_symbol_in_tier;

			tier_first_symbol_index = last_symbol_index;
		}

		return expected_bits_factor;
	}

	void UpdateSymbolEncodings()
	{
		std::size_t tier_first_symbol_index = 0ull;
		for (std::size_t tier_index = 0ull; tier_index < kNumTiers; tier_index++)
		{
			std::size_t num_symbols_in_tier = 1ull << tier_bit_widths_[tier_index];
			// Inclusive
			auto first_symbol_index = tier_first_symbol_index;
			// Non inclusive index
			auto last_symbol_index = std::min(first_symbol_index + num_symbols_in_tier, num_symbols_);

			tier_first_symbol_index = last_symbol_index;

			tier_start_symbol_index_[tier_index] = first_symbol_index;

			for (auto symbol = first_symbol_index; symbol < last_symbol_index; symbol++)
			{
				auto real_symbol = sorted_symbols_[symbol].symbol;
				auto symbol_in_tier = symbol - first_symbol_index;

				symbol_num_bits[real_symbol] = tier_bit_prefix_sizes_[tier_index] + tier_bit_widths_[tier_index];
				symbol_encodings[real_symbol] =
					(tier_bit_prefixes_[tier_index] & ((1ul << tier_bit_prefix_sizes_[tier_index]) - 1ul));
				symbol_encodings[real_symbol] |= (symbol_in_tier & ((1u << tier_bit_widths_[tier_index]) - 1u))
												 << tier_bit_prefix_sizes_[tier_index];
			}
		}
	}

	void FindBestTiers()
	{
		double best_expected_bits_per_symbol = std::numeric_limits<double>::infinity();
		std::size_t best_tier_bit_widths[kNumTiers] = {0};
		std::size_t best_third_tier_single_bit_position = 0;

		auto max_bit_widths_required = 0;
		for (; num_symbols_ > 1ull << max_bit_widths_required; max_bit_widths_required++)
		{
		}

		for (auto first_tier_bit_width = 0; first_tier_bit_width <= max_bit_widths_required; first_tier_bit_width++)
		{
			for (auto second_tier_bit_width = std::min(max_bit_widths_required, first_tier_bit_width + 1);
				 second_tier_bit_width <= max_bit_widths_required; second_tier_bit_width++)
			{
				auto third_tier_bit_width = max_bit_widths_required;

				for (auto third_tier_single_bit_position = 0; third_tier_single_bit_position < 3; third_tier_single_bit_position++)
				{
					SetTierBitWidths(
						first_tier_bit_width, second_tier_bit_width, third_tier_bit_width, third_tier_single_bit_position);
					double expected_bits_per_symbol = CalculateFactorOfExpectedBitsPerSymbol();

					if (expected_bits_per_symbol < best_expected_bits_per_symbol)
					{
						best_expected_bits_per_symbol = expected_bits_per_symbol;
						best_tier_bit_widths[0] = first_tier_bit_width;
						best_tier_bit_widths[1] = second_tier_bit_width;
						best_tier_bit_widths[2] = third_tier_bit_width;
						best_third_tier_single_bit_position = third_tier_single_bit_position;
					}
				}
			}
		}
		SetTierBitWidths(
			best_tier_bit_widths[0], best_tier_bit_widths[1], best_tier_bit_widths[2], best_third_tier_single_bit_position);

		UpdateSymbolEncodings();

		// std::cout << "EXPECTED: " << num_symbols_ << std::endl;
		// std::cout << "\t1: " << tier_bit_widths_[0] << "  " <<
		// tier_bit_prefix_sizes_[0]
		//           << std::endl;
		// std::cout << "\t2: " << tier_bit_widths_[1] << "  " <<
		// tier_bit_prefix_sizes_[1]
		//           << std::endl;
		// std::cout << "\t3: " << tier_bit_widths_[2] << "  " <<
		// tier_bit_prefix_sizes_[2]
		//           << std::endl;
	}

private:
	static const std::size_t kNumTiers = 3;
	struct Symbol
	{
		std::uint64_t symbol;
		std::uint64_t frequency;
	};

	std::size_t num_symbols_;
	std::vector<Symbol> sorted_symbols_;
	std::vector<std::size_t> cumulative_frequencies_of_sorted_symbols_;
	std::array<std::size_t, kNumTiers> tier_bit_widths_;
	std::array<std::size_t, kNumTiers> tier_bit_prefixes_;
	std::array<std::size_t, kNumTiers> tier_bit_prefix_sizes_;
	std::array<std::size_t, kNumTiers> tier_start_symbol_index_;
	std::vector<std::size_t> symbol_num_bits;
	std::vector<std::uint64_t> symbol_encodings;
	std::vector<std::uint64_t> current_symbol_frequencies;

	void SetTierBitWidths(std::size_t first_tier_bit_width, std::size_t second_tier_bit_width, std::size_t third_tier_bit_width,
		std::size_t third_tier_single_bit_position)
	{
		tier_bit_widths_[0] = first_tier_bit_width;
		tier_bit_widths_[1] = second_tier_bit_width;
		tier_bit_widths_[2] = third_tier_bit_width;

		auto num_unique_bit_widths = 1;
		if (first_tier_bit_width != second_tier_bit_width)
		{
			num_unique_bit_widths++;
		}
		if (third_tier_bit_width != second_tier_bit_width)
		{
			num_unique_bit_widths++;
		}

		if (num_unique_bit_widths == 1)
		{
			tier_bit_prefix_sizes_[0] = 0;
			tier_bit_prefix_sizes_[1] = 0;
			tier_bit_prefix_sizes_[2] = 0;
		}
		else if (num_unique_bit_widths == 2)
		{
			tier_bit_prefix_sizes_[0] = 1;
			tier_bit_prefix_sizes_[1] = 1;
			tier_bit_prefix_sizes_[2] = 0;
			tier_bit_prefixes_[0] = 0;
			tier_bit_prefixes_[1] = 1;
		}
		else if (num_unique_bit_widths == 3)
		{
			// TODO try putting single prefix bit elswhere.
			assert(third_tier_single_bit_position < 3);
			switch (third_tier_single_bit_position)
			{
				case 0:
					tier_bit_prefix_sizes_[0] = 1;
					tier_bit_prefix_sizes_[1] = 2;
					tier_bit_prefix_sizes_[2] = 2;
					tier_bit_prefixes_[0] = 0;
					tier_bit_prefixes_[1] = 0b01;
					tier_bit_prefixes_[2] = 0b11;
					break;
				case 1:
					tier_bit_prefix_sizes_[0] = 2;
					tier_bit_prefix_sizes_[1] = 1;
					tier_bit_prefix_sizes_[2] = 2;
					tier_bit_prefixes_[0] = 0b01;
					tier_bit_prefixes_[1] = 0b0;
					tier_bit_prefixes_[2] = 0b11;
					break;
				case 2:
					tier_bit_prefix_sizes_[0] = 2;
					tier_bit_prefix_sizes_[1] = 2;
					tier_bit_prefix_sizes_[2] = 1;
					tier_bit_prefixes_[0] = 0b01;
					tier_bit_prefixes_[1] = 0b11;
					tier_bit_prefixes_[2] = 0b0;
					break;
			}
		}

		assert(tier_bit_widths_[1] >= tier_bit_widths_[0]);
		assert(tier_bit_widths_[2] >= tier_bit_widths_[1]);
	}
};
