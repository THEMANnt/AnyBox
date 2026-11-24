#include "OpusDecoder.hpp"

#include <opus.h>

#include <limits>
#include <stdexcept>
#include <string>

namespace {

void CheckOpus(int status, const char* operation) {
    if (status < 0) throw std::runtime_error(std::string(operation) + ": " + opus_strerror(status));
}

}

void AnyBox::XApu::ReleaseOpusDecoder::operator()(::OpusDecoder* decoder) const noexcept {
    opus_decoder_destroy(decoder);
}

AnyBox::XApu::OpusDecoder::OpusDecoder(std::uint32_t channels, std::uint32_t sampleRate) : channels(channels), sampleRate(sampleRate) {
    if (channels != 1 && channels != 2) throw std::invalid_argument("Opus decoder requires one or two channels");
    if (sampleRate > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) throw std::invalid_argument("Opus sample rate is out of range");

    int status = OPUS_OK;
    decoder.reset(opus_decoder_create(static_cast<int>(sampleRate), static_cast<int>(channels), &status));
    CheckOpus(status, "Opus decoder creation");
    if (!decoder) throw std::bad_alloc();
}

std::uint32_t AnyBox::XApu::OpusDecoder::Decode(std::span<const std::byte> packet, std::span<float> output) {
    if (output.size() % channels != 0) throw std::invalid_argument("Opus output does not contain complete sample frames");
    auto frames = GetFrameCount(packet);
    auto* data = reinterpret_cast<const unsigned char*>(packet.data());
    auto size = static_cast<opus_int32>(packet.size());

    if (static_cast<std::size_t>(frames) > output.size() / channels) throw std::length_error("Opus output buffer is too small");
    int decoded = opus_decode_float(decoder.get(), data, size, output.data(), static_cast<int>(frames), 0);
    CheckOpus(decoded, "Opus packet decoding");

    if (static_cast<std::uint32_t>(decoded) != frames) throw std::runtime_error("Opus decoder returned an inconsistent packet duration");
    return static_cast<std::uint32_t>(decoded);
}

std::uint32_t AnyBox::XApu::OpusDecoder::GetFrameCount(std::span<const std::byte> packet) const {
    if (packet.empty() || packet.size() > static_cast<std::size_t>(std::numeric_limits<opus_int32>::max())) throw std::invalid_argument("Invalid Opus packet size");

    int frames = opus_packet_get_nb_samples(reinterpret_cast<const unsigned char*>(packet.data()), static_cast<opus_int32>(packet.size()), static_cast<opus_int32>(sampleRate));
    CheckOpus(frames, "Opus packet duration");
    return static_cast<std::uint32_t>(frames);
}

void AnyBox::XApu::OpusDecoder::Reset() {
    CheckOpus(opus_decoder_ctl(decoder.get(), OPUS_RESET_STATE), "Opus decoder reset");
}

std::uint32_t AnyBox::XApu::OpusDecoder::GetChannelCount() const {
    return channels;
}

void AnyBox::XApu::OpusDecoder::SetSampleRate(std::uint32_t rate) {
    if (rate == sampleRate) return;
    if (rate != 8000 && rate != 12000 && rate != 16000 && rate != 24000 && rate != 48000) throw std::invalid_argument("Unsupported native Opus sample rate");
    CheckOpus(opus_decoder_init(decoder.get(), static_cast<opus_int32>(rate), static_cast<int>(channels)), "Opus sample rate initialization");
    sampleRate = rate;
}

std::uint32_t AnyBox::XApu::OpusDecoder::GetSampleRate() const {
    return sampleRate;
}
