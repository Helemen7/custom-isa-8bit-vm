#pragma once

#include <cstddef>
#include <filesystem>
#include <fstream>

#include "MemoryManager.hpp"

class Loader {
      public:
	MemAddr load_program(std::filesystem::path path_to_bin,
			     MemoryManager &manager,
			     std::vector<MemCell> &RAM) {
		auto bytecode = read_binary_file(path_to_bin);

		MemAddr first_sector = manager.allocate(bytecode.size());

		for (std::size_t i{0uz}; i < bytecode.size(); ++i) {
			RAM[first_sector + i] = bytecode[i];
		}

		return first_sector;
	}

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
};
