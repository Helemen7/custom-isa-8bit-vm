#pragma once

#include <cstddef>

#include "types.hpp"

struct MemoryBlock {
	MemAddr start_address{};
	std::size_t size{};
	bool operator<(const MemoryBlock &other) const {
		if (start_address != other.start_address)
			return start_address < other.start_address;
		return size < other.size;
	}
};
