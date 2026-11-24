#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

struct AVCodecContext;
struct AVFrame;
struct AVPacket;

namespace AnyBox::Codecs {
struct XmaFrame {
    std::array<std::span<const float>, 2> channels;
};

class XmaFrameDecoder {
public:
    XmaFrameDecoder(std::uint32_t sampleRate, std::uint32_t channels);
    ~XmaFrameDecoder();
    XmaFrameDecoder(XmaFrameDecoder&&) noexcept = default;
    XmaFrameDecoder& operator=(XmaFrameDecoder&&) noexcept = default;
    XmaFrame Decode(std::span<const std::byte> source, std::uint8_t firstBit, std::uint32_t bitCount);
    void Reset();

private:
    struct ContextDeleter { void operator()(AVCodecContext* value) const noexcept; };
    struct FrameDeleter { void operator()(AVFrame* value) const noexcept; };
    struct PacketDeleter { void operator()(AVPacket* value) const noexcept; };
    using Context = std::unique_ptr<AVCodecContext, ContextDeleter>;
    Context createContext() const;
    Context context;
    std::unique_ptr<AVFrame, FrameDeleter> frame;
    std::unique_ptr<AVPacket, PacketDeleter> packet;
    std::uint32_t sampleRate;
    std::uint32_t channels;
    bool needsDrain = false;
    bool failed = false;
};
}
