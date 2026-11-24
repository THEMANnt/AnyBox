#pragma once

#include <immintrin.h>
#include <bit>
#include <cstdint>

inline std::uint32_t AnyBoxBitScanForward(std::uint32_t value) { return static_cast<std::uint32_t>(std::countr_zero(value)); }

#define _tzcnt_u32(value) AnyBoxBitScanForward(value)
