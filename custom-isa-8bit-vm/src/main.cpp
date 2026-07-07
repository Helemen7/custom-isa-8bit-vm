#include <iostream>
#include <map>

#include "Brainfvck.hpp"

#include "../include/Assembler.hpp"
#include "../include/InstructionSignature.hpp"
#include "../include/VM.hpp"
#include "../include/types.hpp"

int main() {
	std::map<InstructionSignature, Opcode> instruction_table;
	instruction_table.insert({InstructionSignature{"NOP", 0}, 0x00});
	instruction_table.insert({InstructionSignature{"MOV", 2}, 0x12});
	instruction_table.insert({InstructionSignature{"PRINT", 1}, 0x01});
	instruction_table.insert({InstructionSignature{"PRINTFLUSH", 0}, 0x10});
	instruction_table.insert({InstructionSignature{"ADD", 2}, 0x32});
	instruction_table.insert({InstructionSignature{"SUB", 2}, 0x52});
	instruction_table.insert({InstructionSignature{"MUL", 2}, 0x72});
	instruction_table.insert({InstructionSignature{"DIV", 2}, 0x92});
	instruction_table.insert({InstructionSignature{"INPUT", 1}, 0x21});
	instruction_table.insert({InstructionSignature{"JMP", 1}, 0x31});
	instruction_table.insert({InstructionSignature{"CMP", 2}, 0xB2});
	instruction_table.insert({InstructionSignature{"JE", 1}, 0x41});
	instruction_table.insert({InstructionSignature{"JNE", 1}, 0x51});
	instruction_table.insert({InstructionSignature{"JG", 1}, 0x61});
	instruction_table.insert({InstructionSignature{"JNG", 1}, 0x71});
	instruction_table.insert({InstructionSignature{"SPUSH", 1}, 0x81});
	instruction_table.insert({InstructionSignature{"SPOP", 1}, 0x91});
	instruction_table.insert({InstructionSignature{"HALLOC", 2}, 0xC2});
	instruction_table.insert({InstructionSignature{"HFREE", 1}, 0xA1});
	instruction_table.insert({InstructionSignature{"CALL", 1}, 0xB1});
	instruction_table.insert({InstructionSignature{"RET", 0}, 0x20});
	instruction_table.insert({InstructionSignature{"FBSET", 2}, 0xD2});
	instruction_table.insert({InstructionSignature{"FBSYNC", 0}, 0x30});

	instruction_table.insert({InstructionSignature{"HALT", {}}, 0xFF});

	const std::unordered_map<std::string_view, uint8_t> regmap = {
	    {"A", 1}, {"B", 2}, {"C", 3}, {"D", 4}};

	Assembler assembler(instruction_table, regmap);
	std::string code{};

	// assembler.asm_file_to_code("../tests/code.asm", code);

	// assembler.asm_to_bin(code, "../tests/code.bin");

	Brainfvck bf(assembler);

	bf.compile("../tests/code.bf", "../tests/code.bin");

	VM vm(instruction_table, regmap);
	vm.set_gui(false);
	auto process{vm.load_program_in_ram("../tests/code.bin")};
	vm.exec(process);
	auto snap = vm.register_snapshot();

	for (auto pair : snap) {
		std::cout << pair.first << " " << static_cast<int>(pair.second)
			  << std::endl;
	}

	return 0;
}
