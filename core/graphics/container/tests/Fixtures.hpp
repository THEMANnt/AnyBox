#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace Fixtures {
inline void Set(std::span<std::byte> data, std::size_t offset, std::uint32_t value) {
    for (std::size_t index = 0; index < 4; ++index) data[offset + index] = static_cast<std::byte>(value >> (index * 8));
}

inline void Require(bool value) {
    if (!value) throw std::runtime_error("DXIL framing test failed");
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    bool caught = false;
    try { callable(); } catch (const TException&) { caught = true; }
    Require(caught);
}

std::vector<std::byte> MakeProgram();
std::vector<std::byte> MakeOptions();
std::vector<std::byte> MakeXboxProgram();
}
