#pragma once

#include <filesystem>
#include <map>
#include <unordered_map>
#include <vector>

#include "InstructionSignature.hpp"
#include "types.hpp"

class Assembler {
      public:
	Assembler(
	    std::map<InstructionSignature, Opcode> &valid_instructions,
	    const std::unordered_map<std::string_view, uint8_t> &_register_map);

	void asm_to_bin(AsmCode code, std::filesystem::path path);
	void asm_file_to_code(std::filesystem::path path, std::string &code);

      private:
	std::map<InstructionSignature, Opcode> &instruction_set;
	std::vector<Opcode>
	instruction_to_opcode(std::size_t line,
			      const AsmTempInstruction &instruction,
			      const std::unordered_map<std::string, MemAddr>
				  &addresses_of_labels);
	const std::unordered_map<std::string_view, uint8_t> &register_map;

	std::vector<Opcode> asm_to_opcodes(AsmCode code);
};
