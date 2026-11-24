#include <anybox/codecs/XmaFrameAssembler.hpp>
#include <algorithm>
#include <stdexcept>

AnyBox::Codecs::XmaPacketResult AnyBox::Codecs::XmaFrameAssembler::Read(std::span<const std::byte> packet, std::uint32_t bitOffset) {
    if (packet.size() != 2048) throw std::length_error("XMA packet must contain 2048 bytes");
    if (bitOffset > 16384 || (bitOffset != 0 && bitOffset < 32)) throw std::out_of_range("XMA frame offset overlaps the packet header");
    if (savedBits != 0 && bitOffset != 0 && bitOffset != 32) throw std::invalid_argument("XMA continuation does not start after the packet header");
    auto first = std::to_integer<std::uint32_t>(packet[0]);
    auto third = std::to_integer<std::uint32_t>(packet[2]);
    auto firstFrame = 32 + ((first & 3) << 13) + (std::to_integer<std::uint32_t>(packet[1]) << 5) + (third >> 3);
    XmaPacketResult result{};
    result.packetDistance = std::to_integer<std::uint32_t>(packet[3]) + 1;
    result.metadata = third & 7;

    if (bitOffset == 0) {
        if (savedBits != 0) bitOffset = 32;
        else {
            if (first >> 2 == 0) return result;
            if (firstFrame >= 16384) throw std::invalid_argument("XMA first frame offset is outside its packet");
            bitOffset = firstFrame;
        }
    }
    if (savedBits < 15) {
        auto count = std::min(15 - savedBits, 16384 - bitOffset);
        append(packet, bitOffset, count);
        bitOffset += count;
        if (savedBits < 15) return result;
        frameBits = (std::to_integer<std::uint32_t>(data[0]) << 7) | (std::to_integer<std::uint32_t>(data[1]) >> 1);
        if (frameBits == 32767) {
            Reset();
            return result;
        }
        if (frameBits < 16) throw std::invalid_argument("XMA frame length is smaller than its header and trailer");
    }

    auto count = std::min(frameBits - savedBits, 16384 - bitOffset);
    append(packet, bitOffset, count);
    bitOffset += count;
    if (savedBits != frameBits) return result;
    result.frameBitCount = frameBits;
    result.frame = std::span(data).first((frameBits + 7) / 8);
    auto finalBit = frameBits - 1;
    auto moreFrames = (std::to_integer<std::uint32_t>(data[finalBit / 8]) >> (7 - finalBit % 8)) & 1;
    if (moreFrames && bitOffset != 16384) {
        result.nextBitOffset = bitOffset;
        result.packetDistance = 0;
    }
    Reset();
    return result;
}
