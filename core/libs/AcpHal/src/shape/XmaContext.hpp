#pragma once

#include <anybox/codecs/XmaFrameAssembler.hpp>
#include <anybox/codecs/XmaFrameDecoder.hpp>
#include <cstdint>
#include <span>
#include <optional>

namespace AnyBox::AcpHal {
class XmaContext {
public:
    XmaContext(std::span<std::uint32_t, 16> words, std::uint32_t index, std::uint64_t& sequence, std::optional<Codecs::XmaFrameDecoder> previous = std::nullopt);
    void Process();
    void Consume(std::uint32_t blocks);
    void Stop() noexcept;
    bool IsActive() const noexcept;
    std::uint64_t GetGeneration() const noexcept;
    std::uint32_t GetFormat() const noexcept;
    Codecs::XmaFrameDecoder TakeDecoder();

private:
    bool readFrame();
    void advancePacket(std::uint32_t nextPacket, std::uint32_t packetCount);
    void writeBlock(std::span<std::byte> output, std::uint32_t writeBlock);
    void validate() const;
    void validateLoop(std::uint32_t buffer, std::uint32_t packetCount) const;
    void configureFrame(bool loopEnd);
    void publishState();
    void validateWork() const;
    std::uint32_t get(std::size_t word, std::uint32_t shift, std::uint32_t mask) const;
    void set(std::size_t word, std::uint32_t shift, std::uint32_t mask, std::uint32_t value);
    std::uint32_t availableBlocks() const;
    std::span<std::uint32_t, 16> words;
    std::uint32_t channels;
    std::uint32_t rateIndex;
    std::uint32_t framePosition = 512;
    std::uint32_t frameLimit = 512;
    std::uint32_t pendingSkip = 0;
    std::uint32_t frameSourceOffset = 0;
    std::uint32_t frameSourceBuffer = 0;
    std::uint32_t skippedPackets = 0;
    std::uint32_t index;
    std::uint64_t& sequence;
    std::uint64_t generation = 0;
    bool active = true;
    std::optional<Codecs::XmaFrameDecoder> decoder;
    Codecs::XmaFrameAssembler assembler;
    Codecs::XmaFrame frame{};
};
}
