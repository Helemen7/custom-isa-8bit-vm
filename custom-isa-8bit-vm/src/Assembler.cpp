#include <algorithm>
#include <charconv>
#include <filesystem>
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
		return false;
	return std::all_of(str.begin(), str.end(), [](unsigned char c) {
		return std::isalpha(c) || c == '_';
	});
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
			if (temp > 0xFF) {
				type_of_args[i - 1] = Type::NUMBER_16;
			} else {
				type_of_args[i - 1] = Type::NUMBER;
			}
		} else {
			// Parsing failed
			std::string arg = instruction[i];

			if (arg.contains('[') && arg.contains(']')) {
				std::string inside =
				    arg.substr(1, arg.size() - 2);

				if (register_map.contains(inside)) {
					type_of_args[i - 1] =
					    Type::INDIRECT_REG;
				} else if (std::isdigit(inside[0]) ||
					   inside[0] == '-') {
					type_of_args[i - 1] =
					    Type::INDIRECT_MEM;
				} else {
					type_of_args[i - 1] =
					    Type::INDIRECT_LBL;
				}
			} else if (std::isdigit(arg[0]) ||
				   (arg[0] == '-' && arg.size() > 1)) {
				type_of_args[i - 1] = Type::NUMBER;
			} else if (register_map.contains(arg)) {
				type_of_args[i - 1] = Type::REGISTER;
			} else {
				type_of_args[i - 1] = Type::ADDRESS;
			}
		}
	}

	InstructionSignature sig(instruction[0], number_of_args);

	auto it{instruction_set.find(sig)};
	if (it == instruction_set.end()) {
		throw InvalidInstructionException(line, instruction[0]);
	}

	std::vector<Opcode> codes;
	codes.push_back(it->second);
	for (auto i{1uz}; i < instruction.size(); ++i) {
		Type type = type_of_args[i - 1];
		codes.push_back(type);
		std::string inside{instruction[i]};
		if (type == Type::INDIRECT_LBL || type == Type::INDIRECT_REG ||
		    type == Type::INDIRECT_MEM) {
			inside = inside.substr(1, inside.size() - 2);
		}
		if (type == Type::ADDRESS || type == Type::INDIRECT_LBL ||
		    type == Type::INDIRECT_MEM) {
			MemAddr addr{};
			if (type != Type::INDIRECT_MEM) {
				auto it = addresses_of_labels.find(inside);
				if (it == addresses_of_labels.end()) {
					throw std::runtime_error(std::format(
					    "Label not found \"{}\"", inside));
				}
				addr = it->second;
			} else {
				// Indirect memory (such as [500]), already
				// contains a memory (offet) address to use
				addr = std::stoull(inside);
			}

			MemCell high_byte{static_cast<MemCell>(
			    (addr >> (sizeof(MemCell) * 8)) & 0xFF)};
			MemCell low_byte{static_cast<MemCell>(addr & 0xFF)};
			codes.push_back(high_byte);
			codes.push_back(low_byte);

		} else if (type == Type::REGISTER ||
			   type == Type::INDIRECT_REG) {
			auto it{register_map.find(inside)};

			if (it != register_map.end()) {
				codes.push_back(it->second);
			} else {
				throw std::runtime_error(
				    "Non existant register used");
			}
		} else if (type == Type::NUMBER) {
			codes.push_back(
			    static_cast<MemCell>(std::stoi(inside)));
		} else if (type == Type::NUMBER_16) {
			int val = std::stoi(inside);

			uint8_t high = static_cast<uint8_t>((val >> 8) & 0xFF);
			uint8_t low = static_cast<uint8_t>(val & 0xFF);

			codes.push_back(static_cast<MemCell>(high));
			codes.push_back(static_cast<MemCell>(low));
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
		auto cmt = line.find(';');
		if (cmt != std::string::npos)
			line = line.substr(0, cmt);

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
				else if (arg.front() == '[' &&
					 arg.back() == ']') {
					std::string inside =
					    arg.substr(1, arg.size() - 2);

					if (register_map.contains(inside)) {
						arg_types.push_back(
						    Type::INDIRECT_REG);
					} else if (std::isdigit(inside[0]) ||
						   inside[0] == '-') {
						arg_types.push_back(
						    Type::INDIRECT_MEM);
					} else {
						arg_types.push_back(
						    Type::INDIRECT_LBL);
					}
				} else {
					arg_types.push_back(Type::NUMBER);
				}
			}
			InstructionSignature sig{first_tok, arg_types.size()};

			if (instruction_set.contains(sig)) {
				std::size_t instruction_size = sizeof(Opcode);
				for (Type t : arg_types) {
					instruction_size +=
					    1; // Type byte for each argument
					instruction_size +=
					    1 + (t == Type::ADDRESS ||
						 t == Type::INDIRECT_LBL ||
						 t == Type::INDIRECT_MEM);
				}
				current_dimension += instruction_size;
			} else {
				throw InvalidInstructionException(line_number,
								  first_tok);
			}
		}
		++line_number;
	}

	code_stream.clear();
	code_stream.seekg(0, std::ios::beg);

	line_number = 1;

	// Set header for program start
	MemAddr high =
	    (addresses_of_labels["start"] << sizeof(MemCell) * 8) & 0xFF;
	MemAddr low = addresses_of_labels["start"] & 0xFF;
	if (addresses_of_labels["start"] ==
	    0) { // If it's still at 0, it means the start tag is not there. we
		 // inizialize it to 2
		low = 0x02;
	}
	codes.push_back(high);
	codes.push_back(low);

	while (std::getline(code_stream, line)) {
		auto cmt = line.find(';');
		if (cmt != std::string::npos)
			line = line.substr(0, cmt);

		if (line.empty())
			continue;
		if (line.back() == ':')
			continue;

		AsmTempInstruction instruction;
		std::string token;

		for (char &c : line)
			if (c == ',')
				c = ' ';

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
}

void Assembler::asm_file_to_code(std::filesystem::path path,
				 std::string &code) {
	std::ifstream file(path);
	std::string text;

	while (std::getline(file, text)) {
		code += text + '\n'; // Add newline
	}
}
