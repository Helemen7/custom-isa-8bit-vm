#include <alloca.h>
#include <cstdint>

#include <cstddef>
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

[[nodiscard]] MemCell &VM::resolve_writeable_arg(Process &proc, Type type,
						 MemAddr arg) {
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

[[nodiscard]] RawArgument VM::fetch_argument(Process &proc) {
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

void VM::spush(RawArgument arg, Process &proc) {
	MemAddr value = (arg.type == Type::NUMBER)
			    ? arg.value
			    : resolve_writeable_arg(proc, arg.type, arg.value);
	std::size_t dim_to_alloc{1uz + (arg.type == Type::ADDRESS ||
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
}

void VM::spop(RawArgument arg, Process &proc) {
	std::size_t dim_to_release{1uz + (arg.type == Type::ADDRESS ||
					  arg.type == Type::INDIRECT_LBL ||
					  arg.type == Type::INDIRECT_MEM)};

	if (proc.SP + dim_to_release >
	    proc.first_sector + proc.allocated_memory) {
		throw std::runtime_error("Stack underflow!");
	}

	if (dim_to_release == 1) {
		resolve_writeable_arg(proc, arg.type, arg.value) = RAM[proc.SP];
	} else {
		MemCell high{RAM[proc.SP]};
		MemCell low{RAM[proc.SP + 1]};

		MemAddr value = (static_cast<MemAddr>(high) << 8) | low;

		resolve_writeable_arg(proc, arg.type, arg.value) = value;
	}

	proc.SP += dim_to_release;
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
			// MOV
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
			// PRINT
			auto el = fetch_argument(proc);
			MemCell value = (el.type == Type::NUMBER)
					    ? static_cast<MemCell>(el.value)
					    : resolve_writeable_arg(
						  proc, el.type, el.value);
			std::cout << static_cast<int>(value);
			break;
		}
		case 0x10: {
			// PRINTFLUSH
			std::cout << std::endl;
			break;
		}
		case 0x32: {
			// ADD
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
			// SUB
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
			// MUL
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
			// DIV
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
			// INPUT
			auto arg = fetch_argument(proc);
			std::cin >> tmp;
			resolve_writeable_arg(proc, arg.type, arg.value) = tmp;
			break;
		}
		case 0x31: {
			// JMP
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
			// CMP
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
			// JE
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
			// JNE
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
			// JG
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
			// JNG
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
			// SPUSH
			auto arg = fetch_argument(proc);

			spush(arg, proc);

			break;
		}
		case 0x91: {
			// SPOP
			// NOTE: Using stack arguments to pass values into a
			// function is not supported, in favour of register and
			// heap use. This is because the CALL instruction puts
			// the function return address in the last stack
			// element, making hypothetical arguments almost
			// impossible to reach without destroying execution
			// stack coherency.
			auto arg = fetch_argument(proc);
			spop(arg, proc);

			break;
		}
		case 0xC2: {
			// HALLOC
			auto place_to_alloc{fetch_argument(proc)};
			auto size_to_alloc{fetch_argument(proc)};

			std::size_t heap_header_block_size{sizeof(MemAddr) +
							   sizeof(HeapReg)};
			MemAddr first_part_of_free_heap = proc.heap_start;

			size_to_alloc.value =
			    (size_to_alloc.type == Type::NUMBER)
				? size_to_alloc.value
				: resolve_writeable_arg(proc,
							size_to_alloc.type,
							size_to_alloc.value);

			bool this_block_invalid{true};

			while (this_block_invalid) {

				if (first_part_of_free_heap >=
				    proc.heap_limit) {
					throw std::runtime_error(
					    "Heap overflow!");
				}

				MemAddr high = RAM[first_part_of_free_heap];
				MemAddr low = RAM[first_part_of_free_heap + 1];
				std::size_t dimension = (high << 8) | low;

				if (RAM[first_part_of_free_heap + 2] ==
				    HeapReg::OWNED) {
					first_part_of_free_heap +=
					    dimension + heap_header_block_size;
				} else {
					if (dimension >= size_to_alloc.value) {
						this_block_invalid = false;

						if (dimension >
						    size_to_alloc.value +
							heap_header_block_size) {
							std::size_t
							    remaining_dimension{
								dimension -
								heap_header_block_size -
								size_to_alloc
								    .value};
							RAM[first_part_of_free_heap +
							    heap_header_block_size +
							    size_to_alloc
								.value] =
							    (remaining_dimension >>
							     8) &
							    0xFF;
							RAM[first_part_of_free_heap +
							    heap_header_block_size +
							    size_to_alloc
								.value +
							    1] =
							    remaining_dimension &
							    0xFF;
							RAM[first_part_of_free_heap +
							    heap_header_block_size +
							    size_to_alloc
								.value +
							    2] = HeapReg::FREE;

						} else {
							size_to_alloc.value =
							    dimension;
						}

						// Insert this block's header
						RAM[first_part_of_free_heap] =
						    (size_to_alloc.value >> 8) &
						    0xFF;
						RAM[first_part_of_free_heap +
						    1] =
						    size_to_alloc.value & 0xFF;
						RAM[first_part_of_free_heap +
						    2] = HeapReg::OWNED;

						first_part_of_free_heap +=
						    heap_header_block_size;
					} else {
						first_part_of_free_heap +=
						    dimension +
						    heap_header_block_size;
					}
				}
			}
			resolve_writeable_arg(proc, place_to_alloc.type,
					      place_to_alloc.value) =
			    first_part_of_free_heap;
			break;
		}
		case 0xA1: {
			// HFREE
			// NOTE: Using a heap memory region after calling HFREE
			// results in undefined behaviour

			auto allocated_region_userstart{fetch_argument(proc)};

			std::size_t heap_header_block_size{sizeof(MemAddr) +
							   sizeof(HeapReg)};

			allocated_region_userstart.value =
			    (allocated_region_userstart.type == Type::ADDRESS)
				? allocated_region_userstart.value
				: resolve_writeable_arg(
				      proc, allocated_region_userstart.type,
				      allocated_region_userstart.value);

			MemAddr actual_regstart{static_cast<MemAddr>(
			    allocated_region_userstart.value -
			    static_cast<MemAddr>(heap_header_block_size))};

			if (RAM[actual_regstart + 2] != HeapReg::OWNED) {
				throw std::runtime_error(
				    "Tried freeing a free heap region");
			}

			RAM[actual_regstart + 2] = HeapReg::FREE;

			MemAddr current_chunk = proc.heap_start;

			while (current_chunk < proc.heap_limit) {
				uint8_t high = RAM[current_chunk];
				uint8_t low = RAM[current_chunk + 1];
				std::size_t dimension =
				    (static_cast<std::size_t>(high) << 8) | low;
				uint8_t status = RAM[current_chunk + 2];

				MemAddr next_chunk = current_chunk +
						     heap_header_block_size +
						     dimension;

				if (next_chunk >= proc.heap_limit) {
					break;
				}

				uint8_t next_high = RAM[next_chunk];
				uint8_t next_low = RAM[next_chunk + 1];
				std::size_t next_dimension =
				    (static_cast<std::size_t>(next_high) << 8) |
				    next_low;
				uint8_t next_status = RAM[next_chunk + 2];

				if (status == HeapReg::FREE &&
				    next_status == HeapReg::FREE) {
					std::size_t new_dimension =
					    dimension + heap_header_block_size +
					    next_dimension;

					RAM[current_chunk] =
					    (new_dimension >> 8) & 0xFF;
					RAM[current_chunk + 1] =
					    new_dimension & 0xFF;

				} else {
					current_chunk = next_chunk;
				}
			}
			break;
		}
		case 0xB1: {
			// CALL
			auto arg{fetch_argument(proc)};
			if (arg.type != Type::ADDRESS) {
				throw std::logic_error(
				    "Cannot call a non-label function");
			}

			MemAddr ret_addr = proc.PC;

			proc.SP -= 2;
			if (proc.SP < proc.stack_limit)
				throw std::runtime_error("Stack overflow!");
			RAM[proc.SP] = (ret_addr >> 8) & 0xFF;
			RAM[proc.SP + 1] = ret_addr & 0xFF;
			proc.PC = arg.value;
			break;
		}
		case 0x20: {
			// RET
			if (proc.SP + 2 >
			    proc.first_sector + proc.allocated_memory)
				throw std::runtime_error("Stack underflow!");
			MemAddr ret_addr =
			    (static_cast<MemAddr>(RAM[proc.SP]) << 8) |
			    RAM[proc.SP + 1];
			proc.SP += 2;
			proc.PC = ret_addr;
			break;
		}

		case 0xFF:
			// HALT
			std::cout
			    << "\n\n--- The program returned control to the "
			       "operating system ---"
			    << std::endl;
			proc.running = false;
			break;
		default:
			// UNK
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
