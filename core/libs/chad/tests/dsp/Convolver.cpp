#include "Tests.hpp"
#include "Convolver.hpp"

#include <complex>
#include <numbers>
#include <vector>

namespace {

void CheckChannels(bool stereoInput, bool stereoFilter, std::uint32_t channels) {
    constexpr std::size_t frames = 16;
    constexpr std::size_t partitions = 9;
    constexpr std::size_t blocks = 20;
    auto inputChannels = stereoInput ? 2U : 1U;
    auto filterChannels = stereoFilter ? 2U : 1U;
    std::vector<float> impulse(filterChannels * frames * partitions);
    std::vector<float> coefficients(filterChannels * partitions * frames * 2);
    for (std::size_t i = 0; i < impulse.size(); ++i) impulse[i] = static_cast<float>(static_cast<int>(i % 13) - 6) / 31.0F;

    for (std::size_t channel = 0; channel < filterChannels; ++channel) {
        for (std::size_t partition = 0; partition < partitions; ++partition) {
            auto begin = (channel * partitions + partition) * frames * 2;
            for (std::size_t bin = 0; bin <= frames; ++bin) {
                std::complex<double> value{};
                for (std::size_t sample = 0; sample < frames; ++sample) {
                    auto angle = -std::numbers::pi * static_cast<double>(bin * sample) / static_cast<double>(frames);
                    value += static_cast<double>(impulse[(channel * partitions + partition) * frames + sample]) * std::polar(1.0, angle);
                }
                if (bin == 0) coefficients[begin] = static_cast<float>(value.real());
                else if (bin == frames) coefficients[begin + 1] = static_cast<float>(value.real());
                else {
                    coefficients[begin + bin * 2] = static_cast<float>(value.real());
                    coefficients[begin + bin * 2 + 1] = static_cast<float>(value.imag());
                }
            }
        }
    }

    AnyBox::Dsp::Convolver convolver(frames, channels, stereoInput, stereoFilter, coefficients);
    std::vector<float> source(inputChannels * blocks * frames);
    for (std::size_t i = 0; i < source.size(); ++i) source[i] = static_cast<float>(static_cast<int>(i % 19) - 9) / 17.0F;
    std::vector<float> input(inputChannels * frames);
    std::vector<float> output(channels * frames + 2, -99.0F);
    for (std::size_t block = 0; block < blocks; ++block) {
        for (std::size_t channel = 0; channel < inputChannels; ++channel) {
            for (std::size_t frame = 0; frame < frames; ++frame) input[channel * frames + frame] = source[(channel * blocks + block) * frames + frame];
        }
        convolver.Process(input, std::span(output).subspan(1, channels * frames));
        AnyBox::Tests::Require(output.front() == -99.0F && output.back() == -99.0F, "Convolver wrote outside its output");
        for (std::size_t channel = 0; channel < channels; ++channel) {
            auto inputChannel = stereoInput ? channel : 0;
            auto filterChannel = stereoFilter ? channel : 0;
            for (std::size_t frame = 0; frame < frames; ++frame) {
                auto position = block * frames + frame;
                double expected = 0.0;
                for (std::size_t tap = 0; tap < frames * partitions && tap <= position; ++tap) {
                    expected += static_cast<double>(impulse[filterChannel * frames * partitions + tap]) * source[inputChannel * blocks * frames + position - tap];
                }
                AnyBox::Tests::Near(output[1 + channel * frames + frame], static_cast<float>(expected), "Partitioned convolution differs from time-domain convolution");
            }
        }
    }
}

}

void AnyBox::Tests::CheckConvolution() {
    CheckChannels(false, false, 1);
    CheckChannels(true, false, 2);
    CheckChannels(false, true, 2);
    CheckChannels(true, true, 2);
}
