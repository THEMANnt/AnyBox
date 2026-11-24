#ifndef ANYBOX_TYPES_HPP
#define ANYBOX_TYPES_HPP

#include <array>
#include <cstdint>

namespace AnyBox {

struct Guid {
    std::uint32_t data1;
    std::uint16_t data2;
    std::uint16_t data3;
    std::array<std::uint8_t, 8> data4;
    bool operator==(const Guid&) const = default;
};

static_assert(sizeof(Guid) == 16);

}

#endif
