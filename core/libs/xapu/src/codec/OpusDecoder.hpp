#ifndef ANYBOX_XAPU_OPUS_DECODER_HPP
#define ANYBOX_XAPU_OPUS_DECODER_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

struct OpusDecoder;

namespace AnyBox::XApu {

struct ReleaseOpusDecoder {
    void operator()(::OpusDecoder* decoder) const noexcept;
};

class OpusDecoder {
public:
    explicit OpusDecoder(std::uint32_t channels, std::uint32_t sampleRate = 48000);
    std::uint32_t Decode(std::span<const std::byte> packet, std::span<float> output);
    std::uint32_t GetFrameCount(std::span<const std::byte> packet) const;
    void Reset();
    void SetSampleRate(std::uint32_t rate);
    std::uint32_t GetSampleRate() const;
    std::uint32_t GetChannelCount() const;

private:
    std::uint32_t channels;
    std::uint32_t sampleRate;
    std::unique_ptr<::OpusDecoder, ReleaseOpusDecoder> decoder;
};

}

#endif
