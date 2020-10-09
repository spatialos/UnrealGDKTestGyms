// Copyright (c) Improbable Worlds Ltd, All Rights Reserved
#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <vector>

class BitPacker
{
public:
	BitPacker()
	{
		Reset();
	}

	void WriteBack(std::uint16_t value, std::size_t bits)
	{
		auto bits_left_in_last_byte_ = 8 - bits_in_current_byte_;
		if (bits <= bits_left_in_last_byte_)
		{
			// This value will fit in the last byte.
			buffer_.back() |= (value & 0xFF) << bits_in_current_byte_;
			bits_in_current_byte_ += bits;
		}
		else
		{
			// This value won't fit. Write what we can then try again.
			buffer_.back() |= (value & 0xFF) << bits_in_current_byte_;
			buffer_.push_back(0);
			bits_in_current_byte_ = 0;
			WriteBack(value >> bits_left_in_last_byte_, bits - bits_left_in_last_byte_);
		}
	}

	void WriteToBuffer(std::vector<std::uint8_t>& output)
	{
		output = buffer_;
	}

	std::size_t NumBitsWritten() const
	{
		return 8 * buffer_.size() - (8u - bits_in_current_byte_);
	}

	void Reset()
	{
		buffer_.clear();
		buffer_.push_back(0);
		bits_in_current_byte_ = 0;
	}

private:
	std::vector<std::uint8_t> buffer_;
	std::size_t bits_in_current_byte_;
};