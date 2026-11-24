#pragma once

#include <anybox/relinker/Lowering.hpp>
#include <initializer_list>

namespace AnyBox::Relinker {
class VectorBuilder {
public:
    void Bytes(std::initializer_list<std::uint8_t> bytes) {
        for (auto value : bytes) code.push_back(static_cast<std::byte>(value));
    }

    void Vector(unsigned prefix, unsigned opcode, unsigned destination, unsigned source) {
        Bytes({static_cast<std::uint8_t>(prefix)});
        auto rex = 0x40 | (destination >= 8 ? 4 : 0) | (source >= 8 ? 1 : 0);
        if (rex != 0x40) Bytes({static_cast<std::uint8_t>(rex)});
        Bytes({0x0F, static_cast<std::uint8_t>(opcode), static_cast<std::uint8_t>(0xC0 | ((destination & 7) << 3) | (source & 7))});
    }

    void Spill(unsigned reg, unsigned offset, bool load) {
        Bytes({0xF3, 0x0F, static_cast<std::uint8_t>(load ? 0x6F : 0x7F), static_cast<std::uint8_t>(0x44 | (reg << 3)), 0x24, static_cast<std::uint8_t>(offset)});
    }

    void Shift(unsigned reg, unsigned operation, unsigned count) {
        Bytes({0x66});
        if (reg >= 8) Bytes({0x41});
        Bytes({0x0F, 0x73, static_cast<std::uint8_t>(0xC0 | (operation << 3) | (reg & 7)), static_cast<std::uint8_t>(count)});
    }

    std::vector<std::byte> code;
};
}
