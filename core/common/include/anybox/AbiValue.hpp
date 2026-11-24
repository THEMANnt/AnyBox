#ifndef ANYBOX_ABI_VALUE_HPP
#define ANYBOX_ABI_VALUE_HPP

#include <array>
#include <bit>
#include <cstddef>
#include <type_traits>

namespace AnyBox {

template<class TValue>
class AbiValue {
public:
    static_assert(std::is_trivially_copyable_v<TValue>);
    TValue Get() const { return std::bit_cast<TValue>(bytes); }
    void Set(TValue value) { bytes = std::bit_cast<decltype(bytes)>(value); }

private:
    std::array<std::byte, sizeof(TValue)> bytes{};
};

}

#endif
