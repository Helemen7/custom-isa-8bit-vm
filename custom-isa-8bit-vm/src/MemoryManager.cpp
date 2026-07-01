#include <algorithm>
#include <cstddef>
#include <format>
#include <stdexcept>

#include "../include/MemoryManager.hpp"

#include "../include/VMconf.hpp"

MemAddr MemoryManager::allocate(std::size_t bytes) {
	MemAddr start_addr{};
	bool found_match{false};

	auto it{free_list.begin()};

	while (it != free_list.end()) {
		if (it->size >= bytes) {
			it->size -= bytes;
			start_addr = it->start_address;
			it->start_address += bytes;
			if (it->size == 0) {
				free_list.erase(it);
			}
			found_match = true;
			break;
		}
		it = std::next(it);
	}
	if (!found_match) {
		throw std::runtime_error(
		    std::format("Allocation of {} bytes failed.", bytes));
	}
	return start_addr;
}

void MemoryManager::free(MemAddr start_addr, std::size_t size) {
	auto it{free_list.begin()};
	while (it != free_list.end()) {
		if (it->start_address < start_addr) {
			it = std::next(it);
		} else {
			break;
		}
	}
	if (it != free_list.begin()) {
		auto it_prev = std::prev(it);
		if (it_prev->start_address + it_prev->size == start_addr) {
			it_prev->size += size;
			if (it != free_list.end() &&
			    it->start_address ==
				it_prev->start_address + it_prev->size) {
				it_prev->size += it->size;
				free_list.erase(it);
			}
		} else if (it != free_list.end() &&
			   it->start_address == start_addr + size) {
			it->size += size;
			it->start_address = start_addr;
		} else {
			MemoryBlock block{start_addr, size};
			free_list.insert(it, block);
		}
	} else {
		if (it != free_list.end() &&
		    it->start_address == start_addr + size) {
			it->size += size;
			it->start_address = start_addr;
		} else {
			MemoryBlock block{start_addr, size};
			free_list.insert(it, block);
		}
	}
}

bool MemoryManager::check_integrity() {
	if (free_list.size() <= 1) {
		return true;
	}

	auto it{free_list.begin()};
	it = std::next(it);
	auto prev_it{free_list.begin()};

	while (it != free_list.end()) {
		if (prev_it->start_address > it->start_address) {
			return false;
		} else if (prev_it->start_address + prev_it->size >
			   it->start_address) {
			return false;
		}
		it = std::next(it);
		prev_it = std::next(prev_it);
	}
	return true;
}

void MemoryManager::reset() {
	free_list.clear();
	free_list.push_back({0, RAM.size() - VMconf::FRAMEBUFFER_TOT_SIZE - 1});
}
