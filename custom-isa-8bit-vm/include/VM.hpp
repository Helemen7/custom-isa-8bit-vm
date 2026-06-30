#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "InstructionSignature.hpp"
#include "Loader.hpp"
#include "MemoryManager.hpp"
#include "RawArgument.hpp"
#include "Type.hpp"
#include "types.hpp"

class VM {
	struct Flags {
		bool zero{false};
		bool sign{false};
	};

      public:
	VM(std::map<InstructionSignature, Opcode> &instruction_set,
	   const std::unordered_map<std::string_view, uint8_t> &_register_map);
	void reset();
	Process load_program_in_ram(std::filesystem::path path_to_bin);
	void exec(Process &proc);
	std::vector<std::pair<std::string_view, MemCell>> register_snapshot();

      private:
	std::array<InstructionSignature, 0xFF + 1> instruction_lookup;
	Loader program_loader;
	std::vector<MemCell> RAM;
	MemoryManager mem_manager;
	std::vector<Register> registers;
	Flags flags;
	const std::unordered_map<std::string_view, uint8_t> &register_map;
	void
	reverse_is(std::map<InstructionSignature, Opcode> &instruction_set);
	MemCell &resolve_writeable_arg(Process &proc, Type type, MemAddr arg);
	RawArgument fetch_argument(Process &proc);
	void spush(RawArgument arg, Process &proc);
	void spop(RawArgument arg, Process &proc);
};
