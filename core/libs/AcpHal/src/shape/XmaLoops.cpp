#include "XmaContext.hpp"
#include <algorithm>
#include <stdexcept>

void AnyBox::AcpHal::XmaContext::validateLoop(std::uint32_t buffer, std::uint32_t packetCount) const {
    auto start = get(3, 0, 0x3FFFFFF);
    auto end = get(4, 0, 0x3FFFFFF);
    if (start % 16384 < 32 || end % 16384 < 32 || start > end || end >= packetCount * 16384) throw std::invalid_argument("Invalid XMA loop frame range");
    if (get(1, 17, 7) > 4) throw std::invalid_argument("Invalid XMA loop start subframe");
    if (start == end && get(1, 17, 7) > get(1, 12, 3)) throw std::invalid_argument("XMA loop does not contain output subframes");
    if (buffer != frameSourceBuffer && assembler.HasPartialFrame()) throw std::invalid_argument("XMA loop spans multiple input buffers");
}

void AnyBox::AcpHal::XmaContext::configureFrame(bool loopEnd) {
    auto initialSkip = get(1, 24, 7);
    auto skipped = std::min(initialSkip, 4u);
    set(1, 24, 7, initialSkip - skipped);
    auto remaining = 4 - skipped;
    auto loopSkip = std::min(pendingSkip, remaining);
    pendingSkip -= loopSkip;
    framePosition = (skipped + loopSkip) * 128;
    frameLimit = 512;
    if (!loopEnd) return;

    frameLimit = (get(1, 12, 3) + 1) * 128;
    set(2, 0, 0x3FFFFFF, get(3, 0, 0x3FFFFFF));
    set(4, 31, 1, frameSourceBuffer);
    auto count = get(0, 12, 255);
    if (count != 255) set(0, 12, 255, count - 1);
    pendingSkip = get(1, 17, 7);
    skippedPackets = 0;
    assembler.Reset();
}
