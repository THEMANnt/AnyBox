#include "Tests.hpp"

#include <array>
#include <exception>
#include <thread>

void AnyBox::Tests::CheckConcurrentStreams() {
    std::vector<float> memory(0x10000 / sizeof(float));
    auto* connection = Connect(memory, 0, 8);
    std::array<std::exception_ptr, 8> errors{};
    std::array<std::jthread, 8> threads;
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::jthread([&, index] {
            try {
                XDspActivationParameters parameters{2, 0, 512, 1, 0, {}};
                XDspStatus* status = nullptr;
                XDspStream* stream = nullptr;
                XDspActivate(connection, &parameters, &status, &stream);
                auto* input = memory.data() + index * 1024;
                auto* output = input + 512;
                for (std::size_t frame = 0; frame < 512; ++frame) input[frame] = static_cast<float>(index + 1);
                XDspCommand command{1.0F, 1.0F, input, output, nullptr};

                for (std::uint32_t iteration = 1; iteration <= 32; ++iteration) {
                    std::uint32_t sequence = 0;
                    Require(XDspSubmitCommand(stream, &command, &sequence) == 0 && sequence == iteration, "Concurrent DSP sequence changed");
                    CheckStatus(status, iteration);
                    Near(output[0], 512.0F * static_cast<float>(index + 1), "Concurrent DSP buffers overlap");
                    for (std::size_t bin = 1; bin < 512; ++bin) Near(output[bin], 0.0F, "Concurrent DSP output was corrupted");
                }
                XDspDeactivate(stream);
            } catch (...) {
                errors[index] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    Require(XDspDisconnect(connection) == 0, "Concurrent DSP client retained active streams");
}
