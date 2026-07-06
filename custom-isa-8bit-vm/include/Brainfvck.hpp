#pragma once

// NOTE: This is a Brainfvck to custom IS ASM adapter. The structure of compiler
// should be as follows: (BRAINFVCK -> ) ASM -> BIN.

// NOTE: The VM/PC executing
// the binary should NOT be told what language the code was originally in. It
// should be a normal binary with the same conventions.

#include "Assembler.hpp"

class Brainfvck {
      public:
	Brainfvck(Assembler &_assembler) : assembler(_assembler) {};
	void compile(std::filesystem::path in, std::filesystem::path out);

      private:
	Assembler &assembler;
};
