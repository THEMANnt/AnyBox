#include "XmaContext.hpp"
#include "../memory/Heap.hpp"
#include <anybox/Unimplemented.hpp>
#include <algorithm>
#include <stdexcept>

void AnyBox::AcpHal::XmaContext::advancePacket(std::uint32_t nextPacket, std::uint32_t packetCount) {
    if (nextPacket < packetCount) {
        set(2, 0, 0x3FFFFFF, nextPacket * 16384);
        return;
    }
    if (get(0, 12, 255) != 0) ANYBOX_NOT_IMPLEMENTED();
    skippedPackets = nextPacket - packetCount;
    auto buffer = get(4, 31, 1);
    set(0, 20 + buffer, 1, 0);
    set(4, 31, 1, buffer ^ 1);
    set(2, 0, 0x3FFFFFF, 0);
}

bool AnyBox::AcpHal::XmaContext::readFrame() {
    while (true) {
        auto buffer = get(4, 31, 1);
        if (get(0, 20 + buffer, 1) == 0) {
            if (get(0, 20 + (buffer ^ 1), 1) == 0) return false;
            buffer ^= 1;
            set(4, 31, 1, buffer);
            set(2, 0, 0x3FFFFFF, 0);
        }
        auto packetCount = get(buffer, 0, 4095);
        if (packetCount == 0) throw std::invalid_argument("Valid XMA input buffer contains no packets");
        if (get(0, 12, 255) != 0) validateLoop(buffer, packetCount);
        if (skippedPackets != 0) {
            auto nextPacket = skippedPackets;
            skippedPackets = 0;
            advancePacket(nextPacket, packetCount);
            continue;
        }
        auto offset = get(2, 0, 0x3FFFFFF);
        auto packetIndex = offset / 16384;
        if (packetIndex >= packetCount) throw std::out_of_range("XMA input offset exceeds its buffer");
        auto input = GetHeap().GetMemory(words[5 + buffer], static_cast<std::size_t>(packetCount) * 2048);
        auto packet = input.subspan(static_cast<std::size_t>(packetIndex) * 2048, 2048);
        if (!assembler.HasPartialFrame()) {
            frameSourceBuffer = buffer;
            frameSourceOffset = offset;
            if (offset % 16384 == 0) {
                auto relative = 32 + ((std::to_integer<std::uint32_t>(packet[0]) & 3) << 13) + (std::to_integer<std::uint32_t>(packet[1]) << 5) + (std::to_integer<std::uint32_t>(packet[2]) >> 3);
                frameSourceOffset += relative;
            }
        }
        auto result = assembler.Read(packet, offset % 16384);
        set(4, 26, 31, result.metadata);
        auto loopEnd = !result.frame.empty() && get(0, 12, 255) != 0 && frameSourceOffset == get(4, 0, 0x3FFFFFF);
        if (!loopEnd) {
            if (result.packetDistance != 0) advancePacket(packetIndex + result.packetDistance, packetCount);
            else set(2, 0, 0x3FFFFFF, packetIndex * 16384 + result.nextBitOffset);
        }
        if (result.frame.empty()) continue;

        frame = decoder->Decode(result.frame, 0, result.frameBitCount);
        publishState();
        configureFrame(loopEnd);
        if (framePosition < frameLimit) return true;
    }
}
