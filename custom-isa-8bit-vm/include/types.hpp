#pragma once

#include <cstdint>

#include <string>
#include <vector>

using AsmCode = std::string_view;
using AsmTempInstruction = std::vector<std::string>;
using Register = uint8_t;
using Opcode = uint8_t;
using MemCell = uint8_t;
using MemAddr = uint16_t; // 16-bit address BUS
using PID = uint32_t;
