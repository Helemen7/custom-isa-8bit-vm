#pragma once
#include <list>
#include <vector>

#include "MemoryBlock.hpp"
#include "VMconf.hpp"

class MemoryManager {
      public:
	MemoryManager(std::vector<MemCell> &_RAM) : RAM(_RAM) {
		free_list.push_back({0, RAM.size() -
					    VMconf::FRAMEBUFFER_TOT_SIZE -
					    1}); // Block containing
						 // all RAM
	};
	MemAddr allocate(std::size_t bytes);
	void free(MemAddr start_region, std::size_t bytes);
	bool check_integrity();
	void reset();

      private:
	std::vector<MemCell> &RAM;
	std::list<MemoryBlock> free_list;
};
