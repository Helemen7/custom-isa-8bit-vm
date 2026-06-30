#pragma once

#include <cstddef>
#include <filesystem>

#include "VMconf.hpp"
#include "types.hpp"

class Process {
      public:
	PID pid{};
	bool running{true};

	MemAddr first_sector{};
	MemAddr last_code_sector{};
	MemAddr entry_point{};
	std::size_t allocated_memory{};

	MemAddr PC{};
	MemAddr SP{};
	MemAddr heap_start{};

	MemAddr stack_limit{};
	MemAddr heap_limit{};

	Process(PID _pid, MemAddr base_address, std::size_t bytecode_size,
		MemAddr _entry_point) {
		entry_point = _entry_point;
		first_sector = base_address;
		last_code_sector = base_address + bytecode_size;
		pid = _pid;
		allocated_memory = bytecode_size +
				   VMconf::PROGRAM_HEAP_STARTING_SIZE_BYTES +
				   VMconf::PROGRAM_STACK_SIZE_BYTES;

		PC = first_sector + entry_point;
		heap_start = first_sector + bytecode_size;
		SP = first_sector + allocated_memory;
		stack_limit = SP - VMconf::PROGRAM_STACK_SIZE_BYTES;
		heap_limit =
		    heap_start + VMconf::PROGRAM_HEAP_STARTING_SIZE_BYTES;
	}

	void update(MemAddr base_address) {
		std::size_t bytecode_size{allocated_memory -
					  (heap_limit - heap_start) -
					  VMconf::PROGRAM_STACK_SIZE_BYTES};
		MemAddr old_fs{first_sector};
		MemAddr old_hs{heap_start};

		first_sector = base_address;
		last_code_sector = base_address + bytecode_size;
		allocated_memory = allocated_memory + (heap_limit - heap_start);
		PC = first_sector + (PC - old_fs);
		heap_start = first_sector + bytecode_size;
		SP = first_sector + (SP - old_fs);
		stack_limit = SP - VMconf::PROGRAM_STACK_SIZE_BYTES;
		heap_limit = heap_start + (heap_limit - old_hs);
		// TODO: Move all heap content and fix last partition
	}
};
