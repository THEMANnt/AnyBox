#include <anybox/codecs/XmaFrameDecoder.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
std::pair<std::vector<std::byte>, std::uint32_t> Silence(std::uint32_t channels, std::uint8_t firstBit) {
    std::vector<bool> bits(15, false);
    bits.push_back(true);
    bits.push_back(false);
    if (channels == 2) bits.push_back(false);
    bits.insert(bits.end(), 8, false);
    bits.insert(bits.end(), 3, false);
    if (channels == 2) {
        bits.push_back(false);
        bits.push_back(true);
        bits.push_back(false);
    }
    bits.insert(bits.end(), channels + 2, false);
    auto bitCount = static_cast<std::uint32_t>(bits.size());
    for (std::uint32_t bit = 0; bit < 15; ++bit) bits[bit] = ((bitCount >> (14 - bit)) & 1) != 0;
    std::vector<std::byte> data((firstBit + bits.size() + 7) / 8);
    for (std::size_t bit = 0; bit < bits.size(); ++bit) {
        if (bits[bit]) data[(firstBit + bit) / 8] |= static_cast<std::byte>(1u << (7 - (firstBit + bit) % 8));
    }
    return {std::move(data), bitCount};
}

void Verify(const AnyBox::Codecs::XmaFrame& frame, std::uint32_t channels) {
    for (std::uint32_t channel = 0; channel < 2; ++channel) {
        if (frame.channels[channel].size() != (channel < channels ? 512u : 0u)) throw std::runtime_error("Incorrect decoded channel size");
        if (!std::ranges::all_of(frame.channels[channel], [](float value) { return std::isfinite(value) && value == 0; })) throw std::runtime_error("Encoded XMA silence decoded incorrectly");
    }
}

void Run() {
    for (auto sampleRate : {24000u, 32000u, 44100u, 48000u}) {
        for (auto channels : {1u, 2u}) {
            AnyBox::Codecs::XmaFrameDecoder decoder(sampleRate, channels);
            for (std::uint8_t offset = 0; offset < 8; ++offset) {
                auto [data, bitCount] = Silence(channels, offset);
                Verify(decoder.Decode(data, offset, bitCount), channels);
            }
            decoder.Reset();
            auto [data, bitCount] = Silence(channels, 0);
            Verify(decoder.Decode(data, 0, bitCount), channels);
            data[0] ^= std::byte{1};
            bool rejected = false;
            try { decoder.Decode(data, 0, bitCount); } catch (const std::invalid_argument&) { rejected = true; }
            if (!rejected) throw std::runtime_error("Invalid XMA frame length was accepted");
        }
    }
    AnyBox::Codecs::XmaFrameDecoder decoder(48000, 1);
    std::array<std::byte, 3> malformed{std::byte{0}, std::byte{47}, std::byte{0}};
    bool rejected = false;
    try { decoder.Decode(malformed, 0, 23); } catch (const std::runtime_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("XMA decoder accepted a bitstream error after producing PCM");
    auto [data, bits] = Silence(1, 0);
    rejected = false;
    try { decoder.Decode(data, 0, bits); } catch (const std::logic_error&) { rejected = true; }
    if (!rejected) throw std::runtime_error("XMA decoder continued using state after a bitstream error");
    decoder.Reset();
    Verify(decoder.Decode(data, 0, bits), 1);
}
}

int main() {
    try {
        Run();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
