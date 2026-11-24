#include "../memory/Tests.hpp"
#include "XmaPacket.hpp"
#include <IAcpHal.hpp>
#include <algorithm>
#include <cstring>

namespace {
void Submit(IAcpHal& channel, std::uint32_t type, const void* data) {
    AnyBox::Tests::Require(channel.SubmitCommand(type, 0, ~0u, data, 0) == 0, "XMA work state command failed");
    AcpHalMessage message{};
    AnyBox::Tests::Require(channel.PopMessage(&message), "XMA work state command completion was lost");
}

struct Storage {
    void* input = nullptr;
    std::array<void*, 2> outputs{};
    std::array<void*, 2> work{};
    std::uint32_t inputBus = 0;
    std::array<std::uint32_t, 2> outputBus{};
    std::array<std::uint32_t, 2> workBus{};

    Storage() {
        ApuAlloc(&input, &inputBus, 2048, 2048, 1);
        auto packet = MakeXmaSilence(1, 8);
        std::memcpy(input, packet.data(), 2048);
        for (auto index = 0u; index < 2; ++index) {
            ApuAlloc(&outputs[index], &outputBus[index], 5888, 256, 2);
            ApuAlloc(&work[index], &workBus[index], 512, 256, 2);
            std::memset(work[index], 0, 512);
        }
    }

    ~Storage() {
        ApuFree(input);
        for (auto index = 0u; index < 2; ++index) {
            ApuFree(outputs[index]);
            ApuFree(work[index]);
        }
    }

    void Configure(AcpHalShapeContexts& descriptor, std::uint32_t index) const {
        auto* words = static_cast<std::uint32_t*>(descriptor.contexts[4]) + index * 16;
        std::fill_n(words, 16, 0u);
        words[0] = 1 | (1 << 20) | (23 << 22);
        words[1] = (4 << 20) | (3 << 27) | (1u << 31);
        words[2] = 32;
        words[5] = inputBus;
        words[7] = outputBus[index];
        words[8] = workBus[index];
    }
};
}

void CheckXmaWork(IAcpHal& channel, AcpHalShapeContexts& descriptor) {
    using namespace AnyBox::Tests;
    Storage storage;
    storage.Configure(descriptor, 0);
    storage.Configure(descriptor, 1);
    std::uint32_t source = 0, destination = 1;
    Submit(channel, 5, &source);
    auto* words = static_cast<std::uint32_t*>(descriptor.contexts[4]);
    std::array<std::byte, 512> snapshot{};
    std::memcpy(snapshot.data(), storage.work[0], 512);
    Require(std::any_of(snapshot.begin(), snapshot.end(), [](auto value) { return value != std::byte{}; }), "XMA work state was not published");
    std::memcpy(storage.work[1], snapshot.data(), 512);
    ExpectException<std::runtime_error>([&] { channel.SubmitCommand(5, 0, ~0u, &destination, 0); });
    Require(channel.GetNumMessages() == 0, "Rejected active XMA state copy produced a completion");
    std::array<std::uint32_t, 2> consumed{0, 4};
    Submit(channel, 18, consumed.data());
    Require(std::memcmp(storage.work[0], snapshot.data(), 512) != 0, "XMA state generation did not advance with decoding");
    Submit(channel, 7, &source);
    ExpectException<std::runtime_error>([&] { channel.SubmitCommand(5, 0, ~0u, &destination, 0); });
    Require(channel.GetNumMessages() == 0, "Rejected stale XMA state copy produced a completion");

    std::memcpy(storage.work[1], storage.work[0], 512);
    words[17] |= 1 << 29;
    ExpectException<std::invalid_argument>([&] { channel.SubmitCommand(5, 0, ~0u, &destination, 0); });
    words[17] &= ~(1u << 29);
    Submit(channel, 5, &destination);
    Require((words[16] >> 27) == 0 && (words[17] >> 31) == 0, "Restored XMA context did not fill its output ring");
    auto* samples = static_cast<const std::byte*>(storage.outputs[1]);
    Require(std::all_of(samples, samples + 5888, [](auto value) { return value == std::byte{}; }), "Restored XMA context produced invalid PCM");
    Submit(channel, 7, &destination);
    ExpectException<std::runtime_error>([&] { channel.SubmitCommand(5, 0, ~0u, &source, 0); });

    std::memcpy(storage.work[0], storage.work[1], 512);
    storage.Configure(descriptor, 0);
    Submit(channel, 5, &source);
    Submit(channel, 7, &source);
    std::memset(storage.work[1], 0x37, 512);
    ExpectException<std::runtime_error>([&] { channel.SubmitCommand(5, 0, ~0u, &destination, 0); });
    Require(channel.GetNumMessages() == 0, "Rejected foreign XMA state produced a completion");
}
