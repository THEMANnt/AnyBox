#include "../memory/Tests.hpp"

#include <IAcpHal.hpp>

#include <array>
#include <exception>
#include <limits>
#include <set>
#include <thread>

void CheckConcurrentChannel() {
    using namespace AnyBox::Tests;
    IAcpHal* channel = nullptr;
    AcpHalCreate(&channel);
    channel->Connect(512, 1024);
    std::array<std::exception_ptr, 8> errors{};
    std::array<std::jthread, 8> threads;
    for (std::size_t i = 0; i < threads.size(); ++i) {
        threads[i] = std::jthread([channel, &errors, i] {
            try {
                std::uint32_t samples = 48000;
                for (std::uint64_t j = 0; j < 64; ++j) {
                    channel->SubmitCommand(2, i * 64 + j, std::numeric_limits<std::uint32_t>::max(), &samples, 0);
                }
            } catch (...) {
                errors[i] = std::current_exception();
            }
        });
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    std::set<std::uint64_t> completed;
    AcpHalMessage message{};
    for (std::size_t i = 0; i < 512; ++i) {
        Require(channel->PopMessage(&message) && message.type == 0x10, "Concurrent ACP completion was lost");
        Require(completed.insert(message.id.Get()).second, "Concurrent ACP completion was duplicated");
    }
    Require(*completed.begin() == 0 && *completed.rbegin() == 511, "Concurrent ACP completion identifier changed");
    Require(channel->Disconnect() == 0, "ACP disconnect did not return success");
    channel->Release();
}
