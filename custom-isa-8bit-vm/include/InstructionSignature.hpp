#pragma once

#include <string_view>
#include <vector>

#include "Type.hpp"

struct InstructionSignature {
	InstructionSignature(std::string_view n, std::vector<Type> t)
	    : name(std::move(n)), arg_types(std::move(t)) {}
	InstructionSignature() : name(""), arg_types{} {}
	std::string_view name{};
	std::vector<Type> arg_types{};

	bool operator<(const InstructionSignature &other) const {
		if (name != other.name)
			return name < other.name;
		return arg_types < other.arg_types;
	}
};
