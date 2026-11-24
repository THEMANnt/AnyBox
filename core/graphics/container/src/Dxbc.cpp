#include <anybox/graphics/container/Dxbc.hpp>
#include "Compression.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

std::array<std::byte, 16> AnyBox::Graphics::Container::ComputeDxbcChecksum(std::span<const std::byte> data) {
    if (data.size() > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXBC checksum input exceeds the container size limit");
    std::array<std::uint32_t, 4> state{0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
    auto completeBytes = data.size() & ~std::size_t{63};
    for (std::size_t offset = 0; offset < completeBytes; offset += 64) CompressDigest(state, std::span<const std::byte, 64>(data.subspan(offset, 64)));
    auto remainder = data.subspan(completeBytes);
    std::array<std::byte, 64> tail{};

    if (remainder.size() >= 56) {
        std::ranges::copy(remainder, tail.begin());
        tail[remainder.size()] = std::byte{0x80};
        CompressDigest(state, tail);
        tail.fill(std::byte{});
    } else {
        std::ranges::copy(remainder, tail.begin() + 4);
        tail[remainder.size() + 4] = std::byte{0x80};
    }

    auto size = static_cast<std::uint32_t>(data.size());
    for (std::size_t index = 0; index < 4; ++index) {
        tail[index] = static_cast<std::byte>((size << 3) >> (index * 8));
        tail[60 + index] = static_cast<std::byte>(((size << 1) | 1) >> (index * 8));
    }
    CompressDigest(state, tail);
    std::array<std::byte, 16> output{};
    for (std::size_t index = 0; index < output.size(); ++index) output[index] = static_cast<std::byte>(state[index / 4] >> ((index % 4) * 8));
    return output;
}
