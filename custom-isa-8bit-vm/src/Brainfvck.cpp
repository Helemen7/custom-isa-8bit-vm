#include <algorithm>
#include <charconv>
#include <format>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "../include/Brainfvck.hpp"

#include "../include/Assembler.hpp"

void Brainfvck::compile(std::filesystem::path in, std::filesystem::path out) {
	std::string code_tmp{".skip 30000\nstart:\nMOV A, 2\n"};
	code_tmp.reserve(65536);

	std::ifstream code_stream(in);

	std::string line{};

	std::size_t number_of_par{0uz};
	std::vector<std::size_t> par_stack;

	while (std::getline(code_stream, line)) {
		for (char c : line) {
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
					    std::to_string(par_id) + ":\n";

				code_tmp += "CMP [A], 0\n";

				code_tmp += "JE close_par_" +
					    std::to_string(par_id) + "\n";

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
					    std::to_string(par_id) + ":\n";

				code_tmp += "CMP [A], 0\n";

				code_tmp += "JNE start_par_" +
					    std::to_string(par_id) + "\n";

				break;
			}
			default: {
			}
			}
		}
	}

	if (!par_stack.empty()) {
		throw std::runtime_error("Unmatched '[' in Brainfvck code");
	}
	code_tmp += "HALT\n";

	apply_optimizations(code_tmp);
	apply_loop_optimizations(code_tmp);

	std::string_view code_view{code_tmp};
	assembler.asm_to_bin(code_view, out);
}

std::optional<int> Brainfvck::parse_par_id(const std::string &line,
					   const std::string &prefix) {
	if (!line.starts_with(prefix) || line.back() != ':')
		return std::nullopt;
	std::string num =
	    line.substr(prefix.size(), line.size() - prefix.size() - 1);
	int id;
	if (auto r = std::from_chars(num.data(), num.data() + num.size(), id);
	    r.ec != std::errc{})
		return std::nullopt;
	return id;
}

std::optional<int> Brainfvck::parse_instr_arg(const std::string &line,
					      const std::string &prefix) {
	if (!line.starts_with(prefix))
		return std::nullopt;
	int n;
	if (auto r = std::from_chars(line.data() + prefix.size(),
				     line.data() + line.size(), n);
	    r.ec != std::errc{})
		return std::nullopt;
	return n;
}

std::optional<Brainfvck::ParsedLoop> Brainfvck::try_parse_loop(const std::vector<std::string> &lines,
							       std::size_t i)
{
	auto par = parse_par_id(lines[i], "start_par_");
	if (!par)
		return std::nullopt;

	int id = *par;
	std::string close_label = "close_par_" + std::to_string(id);

	if (i + 6 >= lines.size())
		return std::nullopt;
	if (lines[i + 1] != "CMP [A], 0" || lines[i + 2] != "JE " + close_label)
		return std::nullopt;

	std::size_t close_idx = i + 3;
	while (close_idx < lines.size() && lines[close_idx] != close_label + ":")
		++close_idx;

	if (close_idx >= lines.size() || close_idx + 2 >= lines.size())
		return std::nullopt;
	if (lines[close_idx + 1] != "CMP [A], 0" ||
	    lines[close_idx + 2] != "JNE start_par_" + std::to_string(id))
		return std::nullopt;

	std::vector<std::string> body(lines.begin() + i + 3, lines.begin() + close_idx);
	return ParsedLoop{id, close_idx, std::move(body)};
}

std::optional<std::vector<Brainfvck::CellMod>>
Brainfvck::try_analyze_mult_body(const std::vector<std::string> &body)
{
	std::vector<CellMod> mods;
	int ptr = 0;

	for (const auto &ln : body) {
		if (auto n = parse_instr_arg(ln, "ADD A, ")) {
			ptr += *n;
		} else if (auto n = parse_instr_arg(ln, "SUB A, ")) {
			ptr -= *n;
		} else if (auto n = parse_instr_arg(ln, "ADD [A], ")) {
			auto it = std::ranges::find_if(mods, [&](const CellMod &m) { return m.offset == ptr; });
			if (it != mods.end())
				it->value += *n;
			else
				mods.push_back({ptr, *n});
		} else if (auto n = parse_instr_arg(ln, "SUB [A], ")) {
			auto it = std::ranges::find_if(mods, [&](const CellMod &m) { return m.offset == ptr; });
			if (it != mods.end())
				it->value -= *n;
			else
				mods.push_back({ptr, -*n});
		} else {
			return std::nullopt;
		}
	}

	if (ptr != 0)
		return std::nullopt;

	auto self = std::ranges::find_if(mods, [](const CellMod &m) { return m.offset == 0; });
	if (self == mods.end() || self->value != -1)
		return std::nullopt;
	mods.erase(self);

	if (mods.empty())
		return std::nullopt;

	return mods;
}

void Brainfvck::emit_mul_replacement(std::vector<std::string> &out,
				     const std::vector<CellMod> &mods)
{
	out.push_back("MOV B, [A]");
	out.push_back("MOV [A], 0");

	auto sorted = mods;
	std::ranges::sort(sorted, [](const CellMod &a, const CellMod &b) { return a.offset < b.offset; });

	int cur = 0;
	for (std::size_t i = 0; i < sorted.size(); ++i) {
		const auto &m = sorted[i];

		int d = m.offset - cur;
		if (d > 0)
			out.push_back(std::format("ADD A, {}", d));
		else if (d < 0)
			out.push_back(std::format("SUB A, {}", -d));

		if (m.value == 1 || m.value == -1) {
			if (m.value == 1)
				out.push_back("ADD [A], B");
			else
				out.push_back("SUB [A], B");
		} else {
			bool last = (i == sorted.size() - 1);
			if (last) {
				out.push_back(std::format("MUL B, {}", m.value));
				out.push_back("ADD [A], B");
			} else {
				out.push_back("MOV C, B");
				out.push_back(std::format("MUL C, {}", m.value));
				out.push_back("ADD [A], C");
			}
		}
		cur = m.offset;
	}

	if (cur > 0)
		out.push_back(std::format("SUB A, {}", cur));
	else if (cur < 0)
		out.push_back(std::format("ADD A, {}", -cur));
}

void Brainfvck::apply_loop_optimizations(std::string &code)
{
	std::vector<std::string> lines;
	{
		std::stringstream stream(code);
		std::string line;
		while (std::getline(stream, line))
			lines.push_back(line);
	}

	std::vector<std::string> out;
	out.reserve(lines.size());

	for (std::size_t i = 0; i < lines.size(); ++i) {
		auto loop = try_parse_loop(lines, i);
		if (!loop) {
			out.push_back(lines[i]);
			continue;
		}

		if (loop->body.size() == 1 && loop->body[0] == "SUB [A], 1") {
			out.push_back("MOV [A], 0");
			i = loop->close_idx + 2;
			continue;
		}

		if (loop->body.size() >= 2 && loop->body[0] == "SUB [A], 1") {
			auto mods = try_analyze_mult_body(loop->body);
			if (mods) {
				emit_mul_replacement(out, *mods);
				i = loop->close_idx + 2;
				continue;
			}
		}

		out.push_back(lines[i]);
	}

	code.clear();
	for (const auto &l : out)
		code += l + "\n";
}

void Brainfvck::apply_optimizations(std::string &code) {
	std::stringstream stream(code);
	std::string line{};

	std::string optimized_code{};

	bool accumulating_math{false};
	bool accumulating_ptr{false};
	int accumulated_value{0};

	auto flush = [&] {
		if (accumulated_value == 0)
			return;
		if (accumulating_math) {
			if (accumulated_value > 0)
				optimized_code += std::format(
				    "ADD [A], {}\n", accumulated_value);
			else
				optimized_code += std::format(
				    "SUB [A], {}\n", -accumulated_value);
		} else if (accumulating_ptr) {
			if (accumulated_value > 0)
				optimized_code += std::format(
				    "ADD A, {}\n", accumulated_value);
			else
				optimized_code += std::format(
				    "SUB A, {}\n", -accumulated_value);
		}
		accumulated_value = 0;
		accumulating_math = false;
		accumulating_ptr = false;
	};

	while (std::getline(stream, line)) {
		if (line.starts_with("ADD [A]")) {
			if (!accumulating_math && !accumulating_ptr)
				accumulating_math = true;
			if (accumulating_math) {
				++accumulated_value;
				continue;
			}
		} else if (line.starts_with("SUB [A]")) {
			if (!accumulating_math && !accumulating_ptr)
				accumulating_math = true;
			if (accumulating_math) {
				--accumulated_value;
				continue;
			}
		} else if (line.starts_with("ADD A")) {
			if (!accumulating_math && !accumulating_ptr)
				accumulating_ptr = true;
			if (accumulating_ptr) {
				++accumulated_value;
				continue;
			}
		} else if (line.starts_with("SUB A")) {
			if (!accumulating_math && !accumulating_ptr)
				accumulating_ptr = true;
			if (accumulating_ptr) {
				--accumulated_value;
				continue;
			}
		}

		flush();
		optimized_code += line + "\n";
	}

	flush();
	code = optimized_code;
}
