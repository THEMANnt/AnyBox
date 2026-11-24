#pragma once

#include <anybox/graphics/container/RootSignature.hpp>
#include <stdexcept>

namespace AnyBox::Graphics::Container::Root {
class Reader {
public:
    explicit Reader(std::span<const std::byte> data) : data(data) {}
    void Check(std::size_t offset, std::size_t count, std::size_t stride) const {
        if (offset % 4 != 0 || offset > data.size() || count > (data.size() - offset) / stride) throw std::invalid_argument("Root signature data range is invalid");
    }
    std::uint32_t Word(std::size_t offset) const {
        Check(offset, 1, 4);
        std::uint32_t value = 0;
        for (unsigned index = 0; index < 4; ++index) value |= std::to_integer<std::uint32_t>(data[offset + index]) << (index * 8);
        return value;
    }
    RootParameter Parameter(std::size_t offset, bool version1) const;

private:
    std::vector<RootRange> ranges(std::size_t offset, bool version1) const;
    std::span<const std::byte> data;
};
}
