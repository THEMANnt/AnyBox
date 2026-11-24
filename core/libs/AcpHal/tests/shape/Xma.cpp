#include "../memory/Tests.hpp"
#include "XmaPacket.hpp"
#include <IAcpHal.hpp>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>

void CheckXmaBuffers(IAcpHal& channel, AcpHalShapeContexts& descriptor);
void CheckXmaLoops(IAcpHal& channel, AcpHalShapeContexts& descriptor);
void CheckXmaWork(IAcpHal& channel, AcpHalShapeContexts& descriptor);

namespace {
void Submit(IAcpHal& channel, std::uint32_t type, const void* data) {
    using AnyBox::Tests::Require;
    Require(channel.SubmitCommand(type, 0x12345678, std::numeric_limits<std::uint32_t>::max(), data, 0) == 0, "SHAPE command failed");
    AcpHalMessage message{};
    Require(channel.PopMessage(&message) && message.type == 0x10 && message.id.Get() == 0x12345678, "SHAPE command completion changed");
}

void Decode(IAcpHal& channel, AcpHalShapeContexts& descriptor, std::uint32_t channels, std::uint32_t rateIndex, std::uint32_t skip) {
    using namespace AnyBox::Tests;
    void* input = nullptr;
    void* output = nullptr;
    void* work = nullptr;
    std::uint32_t inputBus = 0, outputBus = 0, workBus = 0;
    ApuAlloc(&input, &inputBus, 2048, 2048, 1);
    ApuAlloc(&output, &outputBus, 6144, 256, 2);
    ApuAlloc(&work, &workBus, 512, 256, 2);
    auto packet = MakeXmaSilence(channels, 8);
    std::memcpy(input, packet.data(), packet.size());
    std::memset(output, 0x73, 6144);
    std::memset(work, 0, 512);
    auto* words = static_cast<std::uint32_t*>(descriptor.contexts[4]);
    std::fill_n(words, 16, 0u);
    words[0] = 1 | (1 << 20) | (23 << 22);
    words[1] = (4 << 20) | (skip << 24) | (rateIndex << 27) | ((channels - 1) << 29) | (1u << 31);
    words[2] = 32;
    words[5] = inputBus;
    words[7] = outputBus;
    words[8] = workBus;
    std::uint32_t index = 0;
    Submit(channel, 5, &index);
    Require((words[1] >> 31) == 0 && (words[0] >> 27) == 0, "SHAPE did not fill its output ring");
    auto* bytes = static_cast<const std::byte*>(output);
    Require(std::all_of(bytes, bytes + 5888, [](auto value) { return value == std::byte{}; }), "XMA silence did not produce silent PCM");
    Require(std::all_of(bytes + 5888, bytes + 6144, [](auto value) { return value == std::byte{0x73}; }), "XMA decoder overwrote the output ring guard");
    std::array<std::uint32_t, 2> command{0, 24};
    ExpectException<std::out_of_range>([&] { channel.SubmitCommand(18, 1, ~0u, command.data(), 0); });
    Require(channel.GetNumMessages() == 0, "Rejected SHAPE consumption produced a completion");

    auto totalBlocks = (8 * 4 - skip) * channels;
    command[1] = 1;
    for (std::uint32_t consumed = 0; consumed < totalBlocks; ++consumed) Submit(channel, 18, command.data());
    Require((words[0] & (3u << 20)) == 0, "Completed XMA stream retained a valid input buffer");
    Require((words[1] >> 31) == 1 && (words[0] >> 27) == (words[9] & 31), "Drained XMA output ring is not empty");
    ExpectException<std::out_of_range>([&] { channel.SubmitCommand(18, 1, ~0u, command.data(), 0); });
    Submit(channel, 7, &index);
    ExpectException<std::logic_error>([&] { channel.SubmitCommand(18, 1, ~0u, command.data(), 0); });
    ApuFree(input);
    ApuFree(output);
    ApuFree(work);
}
}

int main() {
    try {
        ApuCreateHeap(0x100000, 0x226E00);
        AcpHalShapeContexts descriptor{};
        descriptor.counts[4] = 2;
        AcpHalAllocateShapeContexts(&descriptor);
        IAcpHal* channel = nullptr;
        AcpHalCreate(&channel);
        channel->Connect(16, 16);
        for (auto channels : {1u, 2u}) {
            for (auto rate : {0u, 1u, 2u, 3u}) {
                for (auto skip : {0u, 1u, 3u, 7u}) Decode(*channel, descriptor, channels, rate, skip);
            }
        }
        CheckXmaBuffers(*channel, descriptor);
        CheckXmaLoops(*channel, descriptor);
        CheckXmaWork(*channel, descriptor);
        channel->Disconnect();
        AcpHalMessage message{};
        channel->PopMessage(&message);
        channel->Release();
        AcpHalReleaseShapeContexts();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
