#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "InstructionSignature.hpp"
#include "types.hpp"

class VM {
	struct Flags {
		bool zero{false};
		bool sign{false};
		bool carry{false}; // TODO: Implement carry
	};

      public:
	VM(std::map<InstructionSignature, Opcode> &instruction_set,
	   const std::unordered_map<std::string_view, uint8_t> &_register_map);
	void reset();
	std::pair<MemCell *, MemCell *>
	load_program_in_ram(std::filesystem::path path_to_bin);
	void exec(MemCell *first_sector, MemCell *last_sector);
	std::vector<std::pair<std::string_view, MemCell>> register_snapshot();

      private:
	std::array<InstructionSignature, 0xFF + 1> instruction_lookup;
	MemCell *PC;
	MemCell *SP;
	std::vector<MemCell> RAM;
	std::vector<Register> registers;
	Flags flags;
	const std::unordered_map<std::string_view, uint8_t> &register_map;
	void single_exec(Opcode instruction);
	void
	reverse_is(std::map<InstructionSignature, Opcode> &instruction_set);
};

// TODO: Make reset function
