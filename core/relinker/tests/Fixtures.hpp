#pragma once

#include <anybox/relinker/Relink.hpp>
#include <cstring>

namespace AnyBox::Relinker::Tests {
template<class TValue>
void Put(std::vector<std::byte>& bytes, std::size_t offset, TValue value) { std::memcpy(bytes.data() + offset, &value, sizeof(value)); }

std::vector<std::byte> Fixture(std::span<const std::uint8_t> code);
void CheckPatchedExecution(const Image& image, std::uint64_t expectedLow);
}
