#include <fstream>
#include <string>

#include "../include/Brainfvck.hpp"

#include "../include/Assembler.hpp"

void Brainfvck::compile(std::filesystem::path in, std::filesystem::path out) {
	// We convert the code to custom IS ASM and we just make a binary with
	// it. Optimizations will be implemented.
	std::string code_tmp{".skip 30000\nstart:\nMOV A, 2\n"};
	code_tmp.reserve(65536);

	std::ifstream code_stream(in);

	std::string line{};

	std::size_t number_of_par{0uz};
	std::vector<std::size_t> par_stack;

	while (std::getline(code_stream, line)) {
		for (char c : line) {
			// Process tags.
			switch (c) {
			case '>': {
				code_tmp += "ADD A, 1\n";
				break;
			}
			case '<': {
				code_tmp += "SUB A, 1\n";
				break;
			}
			case '+': {
				code_tmp += "ADD [A], 1\n";
				break;
			}
			case '-': {
				code_tmp += "SUB [A], 1\n";
				break;
			}
			case '.': {
				code_tmp += "PRINT [A]\n";
				break;
			}
			case ',': {
				code_tmp += "INPUT [A]\n";
				break;
			}
			case '[': {
				std::size_t par_id = number_of_par++;
				par_stack.push_back(par_id);

				code_tmp += "start_par_" +
					    std::to_string(par_id) +
					    ":\n";

				code_tmp += "CMP [A], 0\n";

				code_tmp += "JE close_par_" +
					    std::to_string(par_id) +
					    "\n";

				break;
			}
			case ']': {
				if (par_stack.empty()) {
					throw std::runtime_error(
					    "Unmatched ']' in Brainfvck code");
				}
				std::size_t par_id = par_stack.back();
				par_stack.pop_back();

				code_tmp += "close_par_" +
					    std::to_string(par_id) +
					    ":\n";

				code_tmp += "CMP [A], 0\n";

				code_tmp += "JNE start_par_" +
					    std::to_string(par_id) +
					    "\n";

				break;
			}
			default: {
			} // Ignore all other chars so they can be used as
			  // comments
			}
		}
	}

	if (!par_stack.empty()) {
		throw std::runtime_error(
		    "Unmatched '[' in Brainfvck code");
	}
	code_tmp += "HALT\n";

	std::string_view code_view{code_tmp};
	assembler.asm_to_bin(code_view, out);
}
