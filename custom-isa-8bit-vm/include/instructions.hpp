#pragma once

#define X(name, opcode, arg1, arg2)                                            \
	instruction_table.insert(                                              \
	    {InstructionSignature{name, {arg1, arg2}}, opcode});
