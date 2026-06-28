#include <cstdint>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <unordered_map>

#include "../include/VM.hpp"

#include "../exceptions/InvalidInstructionException.hpp"

#include "../include/VMRegisters.hpp"

VM::VM(std::map<InstructionSignature, Opcode> &instruction_set,
       const std::unordered_map<std::string_view, uint8_t> &_register_map)
    : registers(static_cast<std::size_t>(VMRegisters::COUNT)), RAM(64'000),
      register_map(_register_map) {
	PC = &RAM[0];
	SP = &RAM[0];
	reverse_is(instruction_set);
}

void VM::reverse_is(std::map<InstructionSignature, Opcode> &instruction_set) {
	for (auto key : instruction_set) {
		instruction_lookup[key.second] = key.first;
	}
}

std::pair<MemCell *, MemCell *>
VM::load_program_in_ram(std::filesystem::path path_to_bin) {
	std::ifstream bin_file(path_to_bin, std::ios::binary);
	if (!bin_file) {
		throw std::runtime_error("Error opening code for read");
	}

	if (std::filesystem::file_size(path_to_bin) >
	    (RAM.size() - (SP - &RAM[0]))) {
		throw std::runtime_error(std::format(
		    "Out of memory error: Tried allocating {} "
		    "bytes, while there are only {} bytes of free memory",
		    std::filesystem::file_size(path_to_bin),
		    (RAM.size() - (SP - &RAM[0]))));
	}

	MemCell *first_sector{SP};

	MemCell cell;
	while (bin_file.read(reinterpret_cast<char *>(&cell), sizeof(cell))) {
		*SP = cell;
		++SP;
	}

	MemCell *last_sector{--SP};

	return {first_sector, last_sector};
}

void VM::exec(MemCell *first_sector, MemCell *last_sector) {
	// Bootstrap offset is in first 2 bytes (start label position)
	MemAddr high = *first_sector;
	MemAddr low = *(first_sector + 1);
	MemAddr offset = (high << sizeof(MemCell) * 8) | low;
	PC = first_sector + offset;

	std::size_t row{1uz};
	bool running{true};
	while (running && PC <= last_sector) {
		std::size_t n_args =
		    *PC & 0x0F; // Second digit is argument number

		auto lookup_result =
		    instruction_lookup[static_cast<uint8_t>(*PC)];

		std::vector<uint8_t> args;

		if (lookup_result.arg_types.size() != 0) {
			args.reserve(lookup_result.arg_types.size());

			for (auto i{0uz}; i < n_args; ++i) {
				args[i] = *(PC + 1 + i);
			}
		}
		int tmp{};

		switch (static_cast<uint8_t>(*PC)) {
		case 0x12:
			registers[args[0] - 1] = args[1];
			break;
		case 0x22:
			registers[args[0] - 1] = registers[args[1] - 1];
			break;
		case 0x01:
			std::cout << static_cast<int>(registers[args[0] - 1]);
			break;
		case 0x11:
			std::cout << static_cast<int>(args[0]);
			break;
		case 0x10:
			std::cout << std::endl;
			break;
		case 0x32:
			registers[args[0] - 1] += args[1];
			break;
		case 0x42:
			registers[args[0] - 1] += registers[args[1] - 1];
			break;
		case 0x52:
			registers[args[0] - 1] -= args[1];
			break;
		case 0x62:
			registers[args[0] - 1] -= registers[args[1] - 1];
			break;
		case 0x72:
			registers[args[0] - 1] *= args[1];
			break;
		case 0x82:
			registers[args[0] - 1] *= registers[args[1] - 1];
			break;
		case 0x92:
			if (args[1] == 0) {
				throw std::logic_error(
				    "DIVBYZERO: Cannot divide by zero");
			}
			registers[args[0] - 1] /= args[1];
			break;
		case 0xA2:
			if (registers[args[1] - 1] == 0) {
				throw std::logic_error(
				    "DIVBYZERO: Cannot divide by zero");
			}
			registers[args[0] - 1] /= registers[args[1] - 1];
			break;
		case 0x21:
			std::cin >> tmp;
			registers[args[0] - 1] = tmp;
			break;
		case 0x31: {
			MemCell high{*(PC + 1)};
			MemCell low{*(PC + 2)};
			MemAddr addr{static_cast<MemAddr>((high << 8) | low)};
			PC = first_sector + addr;
			++row;
			continue;
		}
		case 0xB2: {
			int res{static_cast<int>(registers[args[0] - 1]) -
				static_cast<int>(registers[args[1] - 1])};
			flags.zero = (res == 0);
			flags.sign = (res > 0);
			break;
		}
		case 0xC2: {
			int res{static_cast<int>(registers[args[0] - 1]) -
				static_cast<int>(args[1])};
			flags.zero = (res == 0);
			flags.sign = (res > 0);
			break;
		}
		case 0x41:
			if (flags.zero) {
				MemCell high{*(PC + 1)};
				MemCell low{*(PC + 2)};
				MemAddr addr{
				    static_cast<MemAddr>((high << 8) | low)};
				PC = first_sector + addr;
				++row;
				continue;
			} else {
				PC += 3;
				continue;
			}
		case 0x51:
			if (!(flags.zero)) {
				MemCell high{*(PC + 1)};
				MemCell low{*(PC + 2)};
				MemAddr addr{
				    static_cast<MemAddr>((high << 8) | low)};
				PC = first_sector + addr;
				++row;
				continue;
			} else {
				PC += 3;
				continue;
			}
		case 0x61:
			if (flags.sign) {
				MemCell high{*(PC + 1)};
				MemCell low{*(PC + 2)};
				MemAddr addr{
				    static_cast<MemAddr>((high << 8) | low)};
				PC = first_sector + addr;
				++row;
				continue;
			} else {
				PC += 3;
				continue;
			}
		case 0x71:
			if (!(flags.sign)) {
				MemCell high{*(PC + 1)};
				MemCell low{*(PC + 2)};
				MemAddr addr{
				    static_cast<MemAddr>((high << 8) | low)};
				PC = first_sector + addr;
				++row;
				continue;
			} else {
				PC += 3;
				continue;
			}
		case 0xFF:
			std::cout
			    << "\n\n--- The program returned control to the "
			       "operating system ---"
			    << std::endl;
			running = false;
			break;
		default:
			throw InvalidInstructionException(row, *PC);
		}

		PC += 1 + n_args;
		++row;
	}
	if (running) {
		std::cerr << "\n\n--- Warning: program terminated forcefully "
			     "because of unhalted exit ---"
			  << std::endl;
	}

	PC = first_sector; // Push PC back
}

std::vector<std::pair<std::string_view, MemCell>> VM::register_snapshot() {
	std::vector<std::pair<std::string_view, MemCell>> snap;
	for (auto el : register_map) {
		snap.push_back({el.first, registers[el.second - 1]});
	}
	return snap;
}
