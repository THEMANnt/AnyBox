#pragma once

#include "Decoder.hpp"
#include <vector>

namespace AnyBox::Relinker {
struct FunctionRange {
    std::uint32_t begin;
    std::uint32_t end;
    std::uint8_t prologSize;
    std::uint8_t stackSize;
};

struct LoweredFunction {
    std::vector<std::byte> code;
    std::vector<FunctionRange> ranges;
};

struct LoweringSite {
    std::uint32_t address;
    std::uint8_t length;
    LoweredFunction function;
};

LoweredFunction LowerImmediateInsert(const DecodedInstruction& instruction, std::uint8_t stackSize = 40);
LoweredFunction LowerImmediateExtract(const DecodedInstruction& instruction);
}
