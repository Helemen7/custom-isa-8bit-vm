#include <iostream>
#include <map>

#include "../include/Assembler.hpp"
#include "../include/InstructionSignature.hpp"
#include "../include/Type.hpp"
#include "../include/VM.hpp"
#include "../include/types.hpp"

int main() {
	std::map<InstructionSignature, Opcode> instruction_table;
	instruction_table.insert(
	    {InstructionSignature{"MOV", {Type::REGISTER, Type::NUMBER}},
	     0x12});
	instruction_table.insert(
	    {InstructionSignature{"MOV", {Type::REGISTER, Type::REGISTER}},
	     0x22});
	instruction_table.insert(
	    {InstructionSignature{"PRINT", {Type::REGISTER}}, 0x01});
	instruction_table.insert(
	    {InstructionSignature{"PRINT", {Type::NUMBER}}, 0x11});
	instruction_table.insert(
	    {InstructionSignature{"PRINTFLUSH", {}}, 0x10});
	instruction_table.insert(
	    {InstructionSignature{"ADD", {Type::REGISTER, Type::NUMBER}},
	     0x32});
	instruction_table.insert(
	    {InstructionSignature{"ADD", {Type::REGISTER, Type::REGISTER}},
	     0x42});
	instruction_table.insert(
	    {InstructionSignature{"SUB", {Type::REGISTER, Type::NUMBER}},
	     0x52});
	instruction_table.insert(
	    {InstructionSignature{"SUB", {Type::REGISTER, Type::REGISTER}},
	     0x62});
	instruction_table.insert(
	    {InstructionSignature{"MUL", {Type::REGISTER, Type::NUMBER}},
	     0x72});
	instruction_table.insert(
	    {InstructionSignature{"MUL", {Type::REGISTER, Type::REGISTER}},
	     0x82});
	instruction_table.insert(
	    {InstructionSignature{"DIV", {Type::REGISTER, Type::NUMBER}},
	     0x92});
	instruction_table.insert(
	    {InstructionSignature{"DIV", {Type::REGISTER, Type::REGISTER}},
	     0xA2});
	instruction_table.insert(
	    {InstructionSignature{"INPUT", {Type::REGISTER}}, 0x21});
	instruction_table.insert(
	    {InstructionSignature{"JMP", {Type::ADDRESS}}, 0x31});
	instruction_table.insert(
	    {InstructionSignature{"CMP", {Type::REGISTER, Type::REGISTER}},
	     0xB2});
	instruction_table.insert(
	    {InstructionSignature{"CMP", {Type::REGISTER, Type::NUMBER}},
	     0xC2});
	instruction_table.insert(
	    {InstructionSignature{"JE", {Type::ADDRESS}}, 0x41});
	instruction_table.insert(
	    {InstructionSignature{"JNE", {Type::ADDRESS}}, 0x51});
	instruction_table.insert(
	    {InstructionSignature{"JG", {Type::ADDRESS}}, 0x61});
	instruction_table.insert(
	    {InstructionSignature{"JNG", {Type::ADDRESS}}, 0x71});

	instruction_table.insert({InstructionSignature{"HALT", {}}, 0xFF});

	const std::unordered_map<std::string_view, uint8_t> REGISTER_MAP = {
	    {"A", 1}, {"B", 2}, {"C", 3}, {"D", 4}};

	Assembler assembler(instruction_table, REGISTER_MAP);
	std::string code{};

	assembler.asm_file_to_code("../tests/code.asm", code);

	assembler.asm_to_bin(code, "../tests/code.bin");

	VM vm(instruction_table, REGISTER_MAP);
	auto sectors{vm.load_program_in_ram("../tests/code.bin")};
	vm.exec(sectors.first, sectors.second);
	auto snap = vm.register_snapshot();

	for (auto pair : snap) {
		std::cout << pair.first << " " << static_cast<int>(pair.second)
			  << std::endl;
	}

	return 0;
}
