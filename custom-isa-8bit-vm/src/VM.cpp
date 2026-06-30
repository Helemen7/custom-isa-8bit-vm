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

#include "../include/RawArgument.hpp"
#include "../include/Type.hpp"
#include "../include/VMRegisters.hpp"

VM::VM(std::map<InstructionSignature, Opcode> &instruction_set,
       const std::unordered_map<std::string_view, uint8_t> &_register_map)
    : registers(static_cast<std::size_t>(VMRegisters::COUNT)), RAM(64'000),
      register_map(_register_map), mem_manager(RAM) {
	reverse_is(instruction_set);
}

void VM::reverse_is(std::map<InstructionSignature, Opcode> &instruction_set) {
	for (auto key : instruction_set) {
		instruction_lookup[key.second] = key.first;
	}
}

Process VM::load_program_in_ram(std::filesystem::path path_to_bin) {

	Process proc{
	    program_loader.load_program(path_to_bin, mem_manager, RAM)};

	return proc;
}

MemCell &VM::resolve_writeable_arg(Process &proc, Type type, MemAddr arg) {
	if (type == Type::REGISTER) {
		return registers[arg - 1];
	}
	if (type == Type::INDIRECT_LBL || type == Type::INDIRECT_MEM ||
	    type == Type::ADDRESS) {
		MemAddr addr = arg + proc.first_sector;
		if (type == Type::INDIRECT_LBL) {
			MemAddr ptr = (RAM[addr] << 8) | RAM[addr + 1];
			return RAM[proc.first_sector + ptr];
		} else {
			return RAM[addr];
		}
	}
	if (type == Type::INDIRECT_REG) {
		return RAM[proc.first_sector + registers[arg - 1]];
	}
	throw std::runtime_error(std::format(
	    "Unknown type for argument (type {})", static_cast<int>(type)));
}

RawArgument VM::fetch_argument(Process &proc) {
	Type type = static_cast<Type>(RAM[proc.PC]);
	proc.PC++;

	MemAddr value = 0;

	if (type == Type::ADDRESS || type == Type::INDIRECT_LBL ||
	    type == Type::INDIRECT_MEM) {
		uint8_t high = RAM[proc.PC];
		uint8_t low = RAM[proc.PC + 1];
		value = (high << 8) | low;
		proc.PC += 2;
	} else if (type == Type::REGISTER || type == Type::INDIRECT_REG ||
		   type == Type::NUMBER) {
		value = RAM[proc.PC];
		proc.PC += 1;
	}

	return {type, value};
}

void VM::exec(Process &proc) {
	// Bootstrap offset is in first 2 bytes (start label position)
	std::size_t row{1uz};
	while (proc.running && proc.PC < proc.last_code_sector) {
		int tmp{};

		MemAddr current_opcode = RAM[proc.PC];
		++proc.PC; // Go from Opcode byte to first type

		switch (current_opcode) {
		case 0x12: {
			auto dest{fetch_argument(proc)};
			auto src{fetch_argument(proc)};

			MemCell src_val = (src.type == Type::NUMBER)
					      ? static_cast<MemCell>(src.value)
					      : resolve_writeable_arg(
						    proc, src.type, src.value);
			resolve_writeable_arg(proc, dest.type, dest.value) =
			    src_val;
			break;
		}
		case 0x01: {
			auto el = fetch_argument(proc);
			MemCell value = (el.type == Type::NUMBER)
					    ? static_cast<MemCell>(el.value)
					    : resolve_writeable_arg(
						  proc, el.type, el.value);
			std::cout << static_cast<int>(value);
			break;
		}
		case 0x10: {
			std::cout << std::endl;
			break;
		}
		case 0x32: {
			auto a = fetch_argument(proc);
			auto b = fetch_argument(proc);

			MemCell b_val =
			    (b.type == Type::NUMBER)
				? static_cast<MemCell>(b.value)
				: resolve_writeable_arg(proc, b.type, b.value);

			resolve_writeable_arg(proc, a.type, a.value) += b_val;

			break;
		}
		case 0x52: {
			auto a = fetch_argument(proc);
			auto b = fetch_argument(proc);

			MemCell b_val =
			    (b.type == Type::NUMBER)
				? static_cast<MemCell>(b.value)
				: resolve_writeable_arg(proc, b.type, b.value);

			resolve_writeable_arg(proc, a.type, a.value) -= b_val;

			break;
		}
		case 0x72: {
			auto a = fetch_argument(proc);
			auto b = fetch_argument(proc);

			MemCell b_val =
			    (b.type == Type::NUMBER)
				? static_cast<MemCell>(b.value)
				: resolve_writeable_arg(proc, b.type, b.value);

			resolve_writeable_arg(proc, a.type, a.value) *= b_val;

			break;
		}
		case 0x92: {
			auto a = fetch_argument(proc);
			auto b = fetch_argument(proc);

			MemCell b_val =
			    (b.type == Type::NUMBER)
				? static_cast<MemCell>(b.value)
				: resolve_writeable_arg(proc, b.type, b.value);

			if (b_val == 0) {
				throw std::logic_error("Cannot divide by zero");
			}

			resolve_writeable_arg(proc, a.type, a.value) /= b_val;

			break;
		}
		case 0x21: {
			auto arg = fetch_argument(proc);
			std::cin >> tmp;
			resolve_writeable_arg(proc, arg.type, arg.value) = tmp;
			break;
		}
		case 0x31: {
			auto target = fetch_argument(proc);
			MemAddr offset{};
			if (target.type == Type::ADDRESS) {
				offset = target.value;
			} else {
				offset = resolve_writeable_arg(
				    proc, target.type, target.value);
			}
			proc.PC = proc.first_sector + offset;
			break;
		}
		case 0xB2: {
			auto arg1 = fetch_argument(proc);
			auto arg2 = fetch_argument(proc);

			MemCell val1 = (arg1.type == Type::NUMBER)
					   ? static_cast<MemCell>(arg1.value)
					   : resolve_writeable_arg(
						 proc, arg1.type, arg1.value);
			MemCell val2 = (arg2.type == Type::NUMBER)
					   ? static_cast<MemCell>(arg2.value)
					   : resolve_writeable_arg(
						 proc, arg2.type, arg2.value);

			int res{static_cast<int>(val1) -
				static_cast<int>(val2)};
			flags.zero = (res == 0);
			flags.sign = (res > 0);
			break;
		}
		case 0x41: {
			auto arg = fetch_argument(proc);
			if (flags.zero) {
				MemAddr value =
				    (arg.type == Type::NUMBER ||
				     arg.type == Type::ADDRESS)
					? static_cast<MemAddr>(arg.value)
					: resolve_writeable_arg(proc, arg.type,
								arg.value);

				proc.PC = proc.first_sector + value;
				break;
			}
			break;
		}
		case 0x51: {
			auto arg = fetch_argument(proc);
			if (!(flags.zero)) {
				MemAddr value =
				    (arg.type == Type::NUMBER ||
				     arg.type == Type::ADDRESS)
					? static_cast<MemAddr>(arg.value)
					: resolve_writeable_arg(proc, arg.type,
								arg.value);

				proc.PC = proc.first_sector + value;
				break;
			}
			break;
		}
		case 0x61: {
			auto arg = fetch_argument(proc);
			if (flags.sign) {
				MemAddr value =
				    (arg.type == Type::NUMBER ||
				     arg.type == Type::ADDRESS)
					? static_cast<MemAddr>(arg.value)
					: resolve_writeable_arg(proc, arg.type,
								arg.value);

				proc.PC = proc.first_sector + value;
				break;
			}
			break;
		}
		case 0x71: {
			auto arg = fetch_argument(proc);
			if (!(flags.sign)) {
				MemAddr value =
				    (arg.type == Type::NUMBER ||
				     arg.type == Type::ADDRESS)
					? static_cast<MemAddr>(arg.value)
					: resolve_writeable_arg(proc, arg.type,
								arg.value);

				proc.PC = proc.first_sector + value;
				break;
			}
			break;
		}
		case 0x81: {
			auto arg = fetch_argument(proc);
			MemAddr value = (arg.type == Type::NUMBER)
					    ? arg.value
					    : resolve_writeable_arg(
						  proc, arg.type, arg.value);
			std::size_t dim_to_alloc{
			    1uz + (arg.type == Type::ADDRESS ||
				   arg.type == Type::INDIRECT_LBL ||
				   arg.type == Type::INDIRECT_MEM)};

			proc.SP -= dim_to_alloc;

			if (proc.SP < proc.stack_limit) {
				throw std::runtime_error("Stack overflow!");
			}

			if (dim_to_alloc == 1) {
				RAM[proc.SP] = value & 0xFF;
			} else {
				RAM[proc.SP] = (value >> 8) & 0xFF;
				RAM[proc.SP + 1] = value & 0xFF;
			}

			break;
		}
		case 0x91: {
			auto arg = fetch_argument(proc);

			std::size_t dim_to_release{
			    1uz + (arg.type == Type::ADDRESS ||
				   arg.type == Type::INDIRECT_LBL ||
				   arg.type == Type::INDIRECT_MEM)};

			if (proc.SP + dim_to_release >
			    proc.first_sector + proc.allocated_memory) {
				throw std::runtime_error("Stack underflow!");
			}

			if (dim_to_release == 1) {
				resolve_writeable_arg(proc, arg.type,
						      arg.value) = RAM[proc.SP];
			} else {
				MemCell high{RAM[proc.SP]};
				MemCell low{RAM[proc.SP + 1]};

				MemAddr value =
				    (static_cast<MemAddr>(high) << 8) | low;

				resolve_writeable_arg(proc, arg.type,
						      arg.value) = value;
			}

			proc.SP += dim_to_release;
			break;
		}

		case 0xFF:
			std::cout
			    << "\n\n--- The program returned control to the "
			       "operating system ---"
			    << std::endl;
			proc.running = false;
			break;
		default:
			throw InvalidInstructionException(row, current_opcode);
		}

		++row;
	}
	if (proc.running) {
		std::cerr << "\n\n--- Warning: program terminated forcefully "
			     "because of unhalted exit to prevent stack "
			     "corruption ---"
			  << std::endl;
	}

	proc.PC = proc.first_sector; // Push PC back
}

std::vector<std::pair<std::string_view, MemCell>> VM::register_snapshot() {
	std::vector<std::pair<std::string_view, MemCell>> snap;
	for (auto el : register_map) {
		snap.push_back({el.first, registers[el.second - 1]});
	}
	return snap;
}

void VM::reset() {
	// NOTE: Execution of code after VM::reset() results in undefined
	// behaviour and should thus be avoided
	flags = Flags();
	registers = std::vector<Register>(VMRegisters::COUNT);
	mem_manager.reset();
	program_loader.reset();
}
