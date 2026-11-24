#pragma once

#include "Decoder.hpp"
#include <bitset>

namespace AnyBox::Relinker {
struct CpuState {
    std::array<std::uint32_t, 4> basic{};
    std::array<std::uint32_t, 4> structured{};
    std::array<std::uint32_t, 4> extended{};
    std::array<std::uint32_t, 4> save{};
    std::uint64_t enabledState = 0;

    static CpuState Detect();
    bool HasAvx() const;
};

class Cpu {
public:
    explicit Cpu(const CpuState& state);
    bool Supports(const DecodedInstruction& instruction) const;
    void Require(const DecodedInstruction& instruction, std::uint32_t address) const;

private:
    std::bitset<ZYDIS_ISA_SET_MAX_VALUE + 1> supported;
    bool tsc = false;
};
}
