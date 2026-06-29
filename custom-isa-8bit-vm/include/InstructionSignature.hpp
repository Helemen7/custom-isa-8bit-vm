#pragma once

#include <string_view>

struct InstructionSignature {
	InstructionSignature(std::string_view n, std::size_t _n_args)
	    : name(n) {
		n_args = _n_args;
	}
	InstructionSignature() : name("") {}
	std::string_view name{};
	std::size_t n_args{};

	bool operator<(const InstructionSignature &other) const {
		if (name != other.name)
			return name < other.name;
		return n_args < other.n_args;
	}
};
