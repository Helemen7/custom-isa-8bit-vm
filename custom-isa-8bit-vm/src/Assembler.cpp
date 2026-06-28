#include <algorithm>
#include <charconv>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>

#include "../include/Assembler.hpp"

#include "../exceptions/InvalidInstructionException.hpp"

#include "../include/InstructionSignature.hpp"
#include "../include/Type.hpp"
#include "../include/types.hpp"

bool isAllAlpha(const std::string &str) {
	if (str.empty())
		return false; // Define behavior for empty strings
	return std::all_of(str.begin(), str.end(),
			   [](unsigned char c) { return std::isalpha(c); });
}

Assembler::Assembler(
    std::map<InstructionSignature, Opcode> &valid_instructions,
    const std::unordered_map<std::string_view, uint8_t> &_register_map)
    : instruction_set(valid_instructions), register_map(_register_map) {}

std::vector<Opcode> Assembler::instruction_to_opcode(
    std::size_t line, const AsmTempInstruction &instruction,
    const std::unordered_map<std::string, MemAddr> &addresses_of_labels) {
	std::size_t number_of_args{instruction.size() - 1};
	std::vector<Type> type_of_args(number_of_args);
	for (auto i{1uz}; i < instruction.size(); ++i) {
		int temp{};
		MemCell value{};

		auto res{std::from_chars(
		    instruction[i].data(),
		    instruction[i].data() + instruction[i].size(), temp)};
		if (res.ec == std::errc{}) {
			// Parsing succeeded
			type_of_args[i - 1] = Type::NUMBER;
		} else {
			// Parsing failed
			if (instruction[i][0] == '\"') {
				type_of_args[i - 1] = Type::WORD;
			} else if (addresses_of_labels.contains(
				       instruction[i])) {
				type_of_args[i - 1] = Type::ADDRESS;
			} else {
				type_of_args[i - 1] = Type::REGISTER;
			}
		}
	}

	InstructionSignature sig(instruction[0], type_of_args);

	auto it{instruction_set.find(sig)};
	if (it == instruction_set.end()) {
		throw InvalidInstructionException(line, instruction[0]);
	}

	std::vector<Opcode> codes;
	codes.push_back(it->second);
	for (auto i{1uz}; i < instruction.size(); ++i) {
		Type type = type_of_args[i - 1];
		if (type == Type::ADDRESS) {
			auto addr{addresses_of_labels.at(instruction[i])};
			MemCell high_byte{static_cast<MemCell>(
			    (addr >> (sizeof(MemCell) * 8)) & 0xFF)};
			MemCell low_byte{static_cast<MemCell>(addr & 0xFF)};
			codes.push_back(high_byte);
			codes.push_back(low_byte);
		} else if (type == Type::REGISTER) {
			auto it{register_map.find(instruction[i])};
			if (it != register_map.end()) {
				codes.push_back(it->second);
			} else {
				throw std::runtime_error(
				    "Non existant register used");
			}
		} else if (type == Type::NUMBER) {
			codes.push_back(std::stoi(instruction[i]));
		}
	}
	return codes;
}

std::vector<Opcode> Assembler::asm_to_opcodes(AsmCode code) {
	std::size_t line_number{1};
	std::stringstream code_stream(static_cast<std::string>(code));
	std::string line;
	std::vector<Opcode> codes;

	std::size_t current_dimension{
	    2uz}; // Bootstrap address dimension kept in count

	std::unordered_map<std::string, MemAddr> addresses_of_labels{};

	while (std::getline(code_stream, line)) {
		std::string first_tok;
		std::stringstream line_stream(line);
		if (!(line_stream >> first_tok))
			continue;

		if ((first_tok.back()) == ':') {
			first_tok.pop_back();
			addresses_of_labels[first_tok] = current_dimension;
		} else {
			std::string arg;
			std::vector<Type> arg_types;
			while (line_stream >> arg) {
				if (arg.ends_with(','))
					arg.pop_back();

				if (register_map.contains(arg))
					arg_types.push_back(Type::REGISTER);
				else if (addresses_of_labels.contains(arg) ||
					 isAllAlpha(arg))
					arg_types.push_back(Type::ADDRESS);
				else
					arg_types.push_back(Type::NUMBER);
			}
			InstructionSignature sig{first_tok, arg_types};

			if (instruction_set.contains(sig)) {
				std::size_t instruction_size = sizeof(Opcode);
				for (Type t : arg_types) {
					instruction_size +=
					    1 + (t == Type::ADDRESS);
				}
				current_dimension += instruction_size;
			} else {
				throw InvalidInstructionException(line_number,
								  first_tok);
			}
		}
	}

	code_stream.clear();
	code_stream.seekg(0, std::ios::beg);

	// Set header for program start
	MemAddr high =
	    (addresses_of_labels["start"] << sizeof(MemCell) * 8) & 0xFF;
	MemAddr low = addresses_of_labels["start"] & 0xFF;
	codes.push_back(high);
	codes.push_back(low);

	while (std::getline(code_stream, line)) {
		if (line.empty())
			continue;
		if (line.back() == ':')
			continue;

		AsmTempInstruction instruction;
		std::string token;

		for (char &c : line) {
			if (c == ',')
				c = ' ';
		}

		std::stringstream line_stream(line);
		while (line_stream >> token) {
			instruction.push_back(token);
		}

		if (instruction.empty())
			continue;

		if (instruction.back().back() == ':' ||
		    instruction.front().back() == ':') {
			continue;
		}

		auto res{instruction_to_opcode(line_number++, instruction,
					       addresses_of_labels)};
		for (auto part : res) {
			codes.push_back(part);
		}
	}
	return codes;
}

void Assembler::asm_to_bin(AsmCode code, std::filesystem::path path) {
	auto binary_content{asm_to_opcodes(code)};
	std::ofstream outBinFile(path, std::ios::out | std::ios::binary);
	if (!outBinFile) {
		throw std::runtime_error("Error opening file");
	}

	for (Opcode op : binary_content) {
		outBinFile.write(reinterpret_cast<const char *>(&op),
				 sizeof(op));
	}

	outBinFile.close();

	std::cout << std::endl;
}

void Assembler::asm_file_to_code(std::filesystem::path path,
				 std::string &code) {
	std::ifstream file(path);
	std::string text;

	while (std::getline(file, text)) {
		code += text + '\n'; // Add newline
	}
}
