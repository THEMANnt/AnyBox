#include "XmaContext.hpp"
#include "../memory/Heap.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>

void AnyBox::AcpHal::XmaContext::writeBlock(std::span<std::byte> output, std::uint32_t writeBlock) {
    auto block = output.subspan(static_cast<std::size_t>(writeBlock) * 256, 256);
    auto frames = 128 / channels;
    for (std::uint32_t sample = 0; sample < frames; ++sample) {
        for (std::uint32_t channel = 0; channel < channels; ++channel) {
            auto value = frame.channels[channel][framePosition + sample];
            if (!std::isfinite(value)) throw std::runtime_error("XMA decoder produced a nonfinite sample");
            auto scaled = std::nearbyint(value * 32767.0F);
            auto pcm = static_cast<std::int16_t>(std::clamp(scaled, -32768.0F, 32767.0F));
            std::memcpy(block.data() + (sample * channels + channel) * sizeof(pcm), &pcm, sizeof(pcm));
        }
    }
    framePosition += frames;
}

void AnyBox::AcpHal::XmaContext::Process() {
    validate();
    validateWork();
    if (get(1, 31, 1) == 0) return;
    auto capacity = get(0, 22, 31);
    auto output = GetHeap().GetMemory(words[7], static_cast<std::size_t>(capacity) * 256);
    auto available = availableBlocks();
    while (available != 0) {
        if (framePosition >= frameLimit && !readFrame()) return;
        auto write = get(0, 27, 31);
        writeBlock(output, write);
        set(0, 27, 31, (write + 1) % capacity);
        --available;
        if (available == 0) set(1, 31, 1, 0);
    }
}
