#include "OpusDecoder.hpp"

#include <opus.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void ExpectException(TCallable callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected Opus error was not raised");
}

void CheckDecoder(std::uint32_t channels, std::uint32_t frames) {
    int status = OPUS_OK;
    std::unique_ptr<OpusEncoder, decltype(&opus_encoder_destroy)> encoder(opus_encoder_create(48000, static_cast<int>(channels), OPUS_APPLICATION_AUDIO, &status), opus_encoder_destroy);
    Require(status == OPUS_OK && encoder, "Opus test encoder creation failed");
    std::vector<float> input(frames * channels);
    for (std::size_t i = 0; i < frames; ++i) {
        for (std::size_t channel = 0; channel < channels; ++channel) {
            input[i * channels + channel] = static_cast<float>(0.25 * std::sin(static_cast<double>(i) * (channel + 1) * 0.08));
        }
    }
    std::array<unsigned char, 4000> packet;
    int bytes = 0;
    for (int i = 0; i < 8; ++i) {
        bytes = opus_encode_float(encoder.get(), input.data(), static_cast<int>(frames), packet.data(), static_cast<opus_int32>(packet.size()));
        Require(bytes > 0, "Opus test encoder warm-up failed");
    }
    Require(bytes > 0, "Opus test packet encoding failed");
    auto encoded = std::as_bytes(std::span(packet.data(), static_cast<std::size_t>(bytes)));
    std::vector<float> first(input.size() + 2, -99.0F);
    std::vector<float> second(input.size(), 0.0F);
    AnyBox::XApu::OpusDecoder decoder(channels);
    Require(decoder.GetChannelCount() == channels, "Opus channel count changed");
    ExpectException<std::length_error>([&] { decoder.Decode(encoded, std::span(first).subspan(1, input.size() - channels)); });
    Require(decoder.Decode(encoded, std::span(first).subspan(1, input.size())) == frames, "Opus decoded frame count changed");
    Require(first.front() == -99.0F && first.back() == -99.0F, "Opus decoder overran its output");
    Require(std::all_of(first.begin() + 1, first.end() - 1, [](float sample) { return std::isfinite(sample); }), "Opus decoder produced non-finite samples");
    Require(std::any_of(first.begin() + 1, first.end() - 1, [](float sample) { return std::abs(sample) > 0.001F; }), "Opus decoder substituted silence");
    decoder.Reset();
    Require(decoder.Decode(encoded, second) == frames, "Opus decoder failed after reset");
    Require(std::equal(second.begin(), second.end(), first.begin() + 1), "Opus reset retained earlier decoder history");
    ExpectException<std::invalid_argument>([&] { decoder.Decode({}, second); });
    std::array<std::byte, 1> invalid{std::byte{0xFF}};
    ExpectException<std::runtime_error>([&] { decoder.Decode(invalid, second); });
}

}

int main() {
    try {
        for (auto channels : {1U, 2U}) {
            for (auto frames : {120U, 240U, 480U, 960U, 1920U, 2880U}) CheckDecoder(channels, frames);
        }
        ExpectException<std::invalid_argument>([] { AnyBox::XApu::OpusDecoder decoder(0); });
        ExpectException<std::invalid_argument>([] { AnyBox::XApu::OpusDecoder decoder(3); });
        ExpectException<std::runtime_error>([] { AnyBox::XApu::OpusDecoder decoder(1, 44100); });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
