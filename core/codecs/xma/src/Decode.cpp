#include <anybox/codecs/XmaFrameDecoder.hpp>
#include "FFmpeg.hpp"
#include "Logging.hpp"
#include <cstring>

AnyBox::Codecs::XmaFrame AnyBox::Codecs::XmaFrameDecoder::Decode(std::span<const std::byte> source, std::uint8_t firstBit, std::uint32_t bitCount) {
    if (failed) throw std::logic_error("XMA decoder must be reset after a bitstream error");
    if (firstBit > 7 || bitCount < 16 || bitCount >= 32767) throw std::invalid_argument("Invalid XMA frame bit range");
    auto byteCount = (static_cast<std::size_t>(firstBit) + bitCount + 7) / 8;
    if (source.size() < byteCount) throw std::length_error("Incomplete XMA frame");
    std::uint32_t header = 0;
    for (std::uint32_t bit = 0; bit < 15; ++bit) {
        auto position = firstBit + bit;
        header = (header << 1) | ((std::to_integer<std::uint32_t>(source[position / 8]) >> (7 - position % 8)) & 1);
    }
    if (header != bitCount) throw std::invalid_argument("XMA frame length does not match its bit range");

    if (needsDrain) {
        auto result = avcodec_receive_frame(context.get(), frame.get());
        if (result != AVERROR(EAGAIN)) {
            CheckXma(result, "Drain XMA frame decoder");
            throw std::runtime_error("XMA decoder returned multiple frames for one packet");
        }
        needsDrain = false;
    }
    if (!av_buffer_is_writable(packet->buf)) throw std::runtime_error("XMA decoder retained its input packet");
    auto endPadding = byteCount * 8 - firstBit - bitCount;
    packet->data[0] = static_cast<std::uint8_t>((firstBit << 5) | (endPadding << 2));
    std::memcpy(packet->data + 1, source.data(), byteCount);
    packet->size = static_cast<int>(byteCount + 1);
    std::memset(packet->data + packet->size, 0, AV_INPUT_BUFFER_PADDING_SIZE);
    XmaErrorScope errors;
    try {
        CheckXma(avcodec_send_packet(context.get(), packet.get()), "Submit XMA frame");
        needsDrain = true;
        CheckXma(avcodec_receive_frame(context.get(), frame.get()), "Decode XMA frame");
        if (errors.HasError()) throw std::runtime_error("XMA decoder reported an invalid bitstream");
        if (frame->format != AV_SAMPLE_FMT_FLTP || frame->nb_samples != 512 || frame->channels != static_cast<int>(channels) || frame->sample_rate != static_cast<int>(sampleRate)) throw std::runtime_error("XMA decoder returned an inconsistent frame format");
    } catch (...) {
        failed = true;
        throw;
    }

    XmaFrame result{};
    for (std::uint32_t channel = 0; channel < channels; ++channel) result.channels[channel] = {reinterpret_cast<const float*>(frame->extended_data[channel]), 512};
    return result;
}
