#pragma once

#include "Assembler.hpp"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

class Brainfvck {
      public:
	Brainfvck(Assembler &_assembler) : assembler(_assembler) {};
	void compile(std::filesystem::path in, std::filesystem::path out);

      private:
	struct CellMod {
		int offset;
		int value;
	};

	struct ParsedLoop {
		int id;
		std::size_t close_idx;
		std::vector<std::string> body;
	};

	Assembler &assembler;

	void apply_optimizations(std::string &code);
	void apply_loop_optimizations(std::string &code);

	std::optional<int> parse_par_id(const std::string &line,
					const std::string &prefix);
	std::optional<int> parse_instr_arg(const std::string &line,
					   const std::string &prefix);
	std::optional<ParsedLoop> try_parse_loop(const std::vector<std::string> &lines,
						 std::size_t i);
	std::optional<std::vector<CellMod>>
	try_analyze_mult_body(const std::vector<std::string> &body);
	void emit_mul_replacement(std::vector<std::string> &out,
				  const std::vector<CellMod> &mods);
};
