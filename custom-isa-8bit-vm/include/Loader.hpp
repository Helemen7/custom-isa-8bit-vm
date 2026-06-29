#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>

#include "MemoryManager.hpp"
#include "Process.hpp"
#include "VMconf.hpp"

class Loader {
      public:
	Process load_program(std::filesystem::path path_to_bin,
			     MemoryManager &manager,
			     std::vector<MemCell> &RAM) {
		auto bytecode = read_binary_file(path_to_bin);

		std::size_t total_program_size{
		    bytecode.size() + VMconf::PROGRAM_HEAP_STARTING_SIZE_BYTES +
		    VMconf::PROGRAM_STACK_SIZE_BYTES};

		MemAddr first_sector = manager.allocate(total_program_size);

		for (std::size_t i{0uz}; i < bytecode.size(); ++i) {
			RAM[first_sector + i] = bytecode[i];
		}

		MemAddr ep_high{RAM[first_sector]};
		MemAddr ep_low{RAM[first_sector + 1]};

		MemAddr entry_point{static_cast<MemAddr>(
		    (ep_high << 8 * sizeof(Opcode)) | ep_low)};

		Process process(next_ready_pid++, first_sector, bytecode.size(),
				entry_point);
		return process;
	}

	void reset() { next_ready_pid = 1; }

      private:
	std::vector<Opcode>
	read_binary_file(std::filesystem::path path_to_bin) {
		std::ifstream filestream(path_to_bin, std::ios::binary);
		std::vector<Opcode> bytecode;
		Opcode code;
		bytecode.reserve(std::filesystem::file_size(path_to_bin));

		while (filestream.read(reinterpret_cast<char *>(&code),
				       sizeof(Opcode))) {
			bytecode.push_back(code);
		}

		return bytecode;
	}
	PID next_ready_pid{1};
};
