#pragma once

#include <cstdint>

enum Type : uint8_t {
	NUMBER,
	REGISTER,
	ADDRESS,
	INDIRECT_REG,
	INDIRECT_MEM,
	INDIRECT_LBL
};
