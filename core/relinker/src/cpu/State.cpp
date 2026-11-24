#include <anybox/relinker/Cpu.hpp>
#include <bit>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#else
#error AnyBox CPU detection requires Windows
#endif

namespace {
std::array<std::uint32_t, 4> ReadCpu(std::uint32_t leaf, std::uint32_t subleaf = 0) {
#ifdef _MSC_VER
    std::array<int, 4> registers{};
    __cpuidex(registers.data(), static_cast<int>(leaf), static_cast<int>(subleaf));
    return std::bit_cast<std::array<std::uint32_t, 4>>(registers);
#else
    std::array<std::uint32_t, 4> registers{};
    __cpuid_count(leaf, subleaf, registers[0], registers[1], registers[2], registers[3]);
    return registers;
#endif
}
}

AnyBox::Relinker::CpuState AnyBox::Relinker::CpuState::Detect() {
    CpuState result;
    auto basicMaximum = ReadCpu(0)[0];
    auto extendedMaximum = ReadCpu(0x80000000)[0];
    if (basicMaximum >= 1) result.basic = ReadCpu(1);
    if (basicMaximum >= 7) result.structured = ReadCpu(7);
    if (extendedMaximum >= 0x80000001) result.extended = ReadCpu(0x80000001);
    if ((result.basic[2] & (1U << 27)) != 0) {
        result.enabledState = GetEnabledXStateFeatures();
        if (basicMaximum >= 13) result.save = ReadCpu(13, 1);
    }
    return result;
}

bool AnyBox::Relinker::CpuState::HasAvx() const {
    constexpr auto required = (1U << 26) | (1U << 27) | (1U << 28);
    return (basic[2] & required) == required && (enabledState & 6) == 6;
}
