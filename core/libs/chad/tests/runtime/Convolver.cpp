#include "Tests.hpp"

#include <algorithm>

void AnyBox::Tests::CheckConvolutionStream() {
    constexpr std::size_t frames = 512;
    constexpr std::size_t partitions = 9;
    std::vector<float> memory(0x10000 / sizeof(float));
    std::vector<float> coefficients(frames * 2 * partitions);
    for (std::size_t partition : {0U, 8U}) {
        auto gain = partition == 0 ? 1.0F : 0.5F;
        auto begin = partition * frames * 2;
        coefficients[begin] = gain;
        coefficients[begin + 1] = gain;
        for (std::size_t bin = 1; bin < frames; ++bin) coefficients[begin + bin * 2] = gain;
    }

    auto* connection = Connect(memory, 1, 2);
    XDspActivationParameters parameters{1, 0, frames, 1, static_cast<std::uint32_t>(coefficients.size()), {}};
    parameters.impulseResponse.Set(coefficients.data());
    XDspStatus* status = nullptr;
    XDspStream* stream = nullptr;
    Require(XDspActivate(connection, &parameters, &status, &stream) == 0 && stream && status, "DSP convolution activation failed");
    XDspCommand command{1.0F, 1.0F, memory.data(), memory.data() + 1024, nullptr};
    std::uint32_t sequence = 0;
    for (std::uint32_t block = 0; block < 12; ++block) {
        std::fill_n(memory.begin(), frames, 0.0F);
        if (block == 0) memory[3] = 1.0F;
        Require(XDspSubmitCommand(stream, &command, &sequence) == 0 && sequence == block + 1, "DSP convolution submission failed");
        CheckStatus(status, sequence);
        for (std::size_t frame = 0; frame < frames; ++frame) {
            auto expected = frame == 3 ? block == 0 ? 1.0F : block == 8 ? 0.5F : 0.0F : 0.0F;
            Near(memory[1024 + frame], expected, "DSP convolution lost its delayed impulse response");
        }
    }
    Require(XDspDeactivate(stream) == 0, "DSP convolution deactivation failed");
    Require(XDspDisconnect(connection) == 0, "DSP convolution client disconnection failed");
}
