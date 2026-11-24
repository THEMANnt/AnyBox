#pragma once

#include <bit>
#include <cstddef>
#include <cstring>
#include <span>
#include <stdexcept>
#include <type_traits>

namespace AnyBox::Relinker {
template<class TValue>
void Write(std::span<std::byte> bytes, std::size_t offset, TValue value) {
    static_assert(std::is_unsigned_v<TValue> && std::endian::native == std::endian::little);
    if (offset > bytes.size() || sizeof(TValue) > bytes.size() - offset) throw std::runtime_error("PE output field exceeds its file bounds");
    std::memcpy(bytes.data() + offset, &value, sizeof(value));
}

template<class TValue>
TValue Read(std::span<const std::byte> bytes, std::size_t offset) {
    static_assert(std::is_unsigned_v<TValue> && std::endian::native == std::endian::little);
    if (offset > bytes.size() || sizeof(TValue) > bytes.size() - offset) throw std::runtime_error("PE field exceeds the file bounds");
    TValue result;
    std::memcpy(&result, bytes.data() + offset, sizeof(result));
    return result;
}
}
