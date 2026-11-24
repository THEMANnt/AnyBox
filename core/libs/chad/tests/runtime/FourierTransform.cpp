#include "Tests.hpp"

void AnyBox::Tests::CheckFftStreams() {
    std::vector<float> memory(0x10000 / sizeof(float));
    auto* connection = Connect(memory, 0, 2);
    XDspActivationParameters parameters{2, 0, 512, 1, 0, {}};
    XDspStatus* forwardStatus = nullptr;
    XDspStatus* inverseStatus = nullptr;
    XDspStream* forward = nullptr;
    XDspStream* inverse = nullptr;
    Require(XDspActivate(connection, &parameters, &forwardStatus, &forward) == 0, "DSP FFT activation failed");
    parameters.type = 3;
    Require(XDspActivate(connection, &parameters, &inverseStatus, &inverse) == 0, "DSP inverse FFT activation failed");
    Require(forward != inverse && forwardStatus != inverseStatus, "DSP streams share their identity or status");
    ExpectException<std::logic_error>([&] { XDspDisconnect(connection); });
    CheckStatus(forwardStatus, 0);
    CheckStatus(inverseStatus, 0);

    for (std::size_t i = 0; i < 512; ++i) memory[i] = 1.0F + (i % 2 ? -0.25F : 0.25F);
    XDspCommand forwardCommand{1.0F, 1.0F, memory.data(), memory.data() + 1024, nullptr};
    XDspCommand inverseCommand{1.0F, 1.0F, memory.data() + 1024, memory.data() + 2048, nullptr};
    std::uint32_t sequence = 0;
    for (std::uint32_t i = 1; i <= 32; ++i) {
        Require(XDspSubmitCommand(forward, &forwardCommand, &sequence) == 0 && sequence == i, "DSP FFT command counter is incorrect");
        CheckStatus(forwardStatus, i);
        Near(memory[1024], 512.0F, "DSP FFT lost its DC component");
        Near(memory[1025], 128.0F, "DSP FFT lost its Nyquist component");
        Require(XDspSubmitCommand(inverse, &inverseCommand, nullptr) == 0, "DSP inverse FFT command failed");
        CheckStatus(inverseStatus, i);
        for (std::size_t frame = 0; frame < 512; ++frame) Near(memory[2048 + frame], memory[frame], "DSP FFT round trip corrupted PCM samples");
    }

    auto invalid = forwardCommand;
    invalid.input = &invalid.startGain;
    ExpectException<std::invalid_argument>([&] { XDspSubmitCommand(forward, &invalid, nullptr); });
    CheckStatus(forwardStatus, 32);
    Require(XDspDeactivate(forward) == 0 && XDspDeactivate(inverse) == 0, "DSP FFT deactivation failed");
    ExpectException<std::invalid_argument>([&] { XDspSubmitCommand(forward, &forwardCommand, nullptr); });
    Require(XDspDisconnect(connection) == 0, "DSP client disconnection failed");
}
