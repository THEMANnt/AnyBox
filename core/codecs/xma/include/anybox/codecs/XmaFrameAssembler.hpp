#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Codecs {
struct XmaPacketResult {
    std::span<const std::byte> frame;
    std::uint32_t frameBitCount = 0;
    std::uint32_t nextBitOffset = 0;
    std::uint32_t packetDistance = 0;
    std::uint32_t metadata = 0;
};

class XmaFrameAssembler {
public:
    XmaPacketResult Read(std::span<const std::byte> packet, std::uint32_t bitOffset);
    void Reset() noexcept;
    bool HasPartialFrame() const noexcept;

private:
    void append(std::span<const std::byte> source, std::uint32_t offset, std::uint32_t count);
    std::array<std::byte, 4096> data{};
    std::uint32_t savedBits = 0;
    std::uint32_t frameBits = 0;
};
}
