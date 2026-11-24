#include <anybox/graphics/container/View.hpp>
#include <anybox/graphics/container/Dxbc.hpp>
#include "Integer.hpp"
#include <algorithm>
#include <limits>

std::vector<std::byte> AnyBox::Graphics::Container::BuildDxbc(std::span<const Part> parts) {
    constexpr auto maximum = std::numeric_limits<std::uint32_t>::max();
    if (parts.size() > (maximum - 32) / 4) throw std::length_error("DXBC part table exceeds the container size limit");
    std::uint64_t size = 32 + parts.size() * 4;
    for (const auto& part : parts) {
        if (part.data.size() > maximum) throw std::length_error("DXBC part exceeds the container size limit");
        size += 8 + ((static_cast<std::uint64_t>(part.data.size()) + 3) & ~std::uint64_t{3});
        if (size > maximum) throw std::length_error("DXBC data exceeds the container size limit");
    }
    std::vector<std::byte> output(static_cast<std::size_t>(size));
    WriteInteger(output, 0, 0x43425844);
    WriteInteger(output, 20, 1);
    WriteInteger(output, 24, static_cast<std::uint32_t>(size));
    WriteInteger(output, 28, static_cast<std::uint32_t>(parts.size()));
    std::size_t offset = 32 + parts.size() * 4;

    for (std::size_t index = 0; index < parts.size(); ++index) {
        const auto& part = parts[index];
        WriteInteger(output, 32 + index * 4, static_cast<std::uint32_t>(offset));
        WriteInteger(output, offset, part.tag);
        WriteInteger(output, offset + 4, static_cast<std::uint32_t>(part.data.size()));
        std::ranges::copy(part.data, output.begin() + static_cast<std::ptrdiff_t>(offset + 8));
        offset += 8 + ((part.data.size() + 3) & ~std::size_t{3});
    }
    auto checksum = ComputeDxbcChecksum(std::span(output).subspan(20));
    std::ranges::copy(checksum, output.begin() + 4);
    return output;
}
