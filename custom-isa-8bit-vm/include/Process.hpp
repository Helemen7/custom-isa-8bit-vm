#pragma once

#include <cstddef>

#include "VMconf.hpp"
#include "types.hpp"

class Process {
      public:
	PID pid{};
	bool running{true};

	MemAddr first_sector{};
	MemAddr last_code_sector{};
	std::size_t allocated_memory{};

	MemAddr PC{};
	MemAddr SP{};
	MemAddr HP{};

	MemAddr stack_limit{};
	MemAddr heap_limit{};

	Process(PID _pid, MemAddr base_address, std::size_t bytecode_size,
		MemAddr entry_point) {
		first_sector = base_address;
		last_code_sector = base_address + bytecode_size;
		pid = _pid;
		allocated_memory = bytecode_size +
				   VMconf::PROGRAM_HEAP_STARTING_SIZE_BYTES +
				   VMconf::PROGRAM_STACK_SIZE_BYTES;

		PC = first_sector + entry_point;
		HP = first_sector + bytecode_size;
		SP = first_sector + allocated_memory;
		stack_limit = SP - VMconf::PROGRAM_STACK_SIZE_BYTES;
		heap_limit = HP + VMconf::PROGRAM_HEAP_STARTING_SIZE_BYTES;
	}
};
