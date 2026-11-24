#include "../memory/Tests.hpp"
#include "XmaPacket.hpp"
#include <IAcpHal.hpp>
#include <algorithm>
#include <cstring>

namespace {
void Submit(IAcpHal& channel, std::uint32_t type, const void* data) {
    using AnyBox::Tests::Require;
    Require(channel.SubmitCommand(type, 0, ~0u, data, 0) == 0, "XMA loop command failed");
    AcpHalMessage message{};
    Require(channel.PopMessage(&message), "XMA loop completion was lost");
}

void Loop(IAcpHal& channel, AcpHalShapeContexts& descriptor, std::uint32_t channels, std::uint32_t repetitions, std::uint32_t startSkip, std::uint32_t endSubframe, bool sameFrame) {
    using namespace AnyBox::Tests;
    void* input = nullptr;
    void* output = nullptr;
    void* work = nullptr;
    std::uint32_t inputBus = 0, outputBus = 0, workBus = 0;
    ApuAlloc(&input, &inputBus, 2048, 2048, 1);
    ApuAlloc(&output, &outputBus, 5888, 256, 2);
    ApuAlloc(&work, &workBus, 512, 256, 2);
    auto packet = MakeXmaSilence(channels, 8);
    std::memcpy(input, packet.data(), 2048);
    std::memset(output, 0x73, 5888);
    std::memset(work, 0, 512);
    auto* words = static_cast<std::uint32_t*>(descriptor.contexts[4]);
    std::fill_n(words, 16, 0u);
    words[0] = 1 | (repetitions << 12) | (1 << 20) | (23 << 22);
    words[1] = (endSubframe << 12) | (startSkip << 17) | (4 << 20) | (3 << 27) | ((channels - 1) << 29) | (1u << 31);
    words[2] = 32;
    auto size = channels == 1 ? 31u : 36u;
    auto endFrame = sameFrame ? 1u : 3u;
    words[3] = 32 + size;
    words[4] = 32 + endFrame * size;
    words[5] = inputBus;
    words[7] = outputBus;
    words[8] = workBus;
    std::uint32_t index = 0;
    Submit(channel, 5, &index);
    Require((words[1] >> 31) == 0, "XMA loop did not fill its output ring");
    auto loopSubframes = endFrame * 4 + endSubframe + 1 - 4 - startSkip;
    auto totalBlocks = (32 + repetitions * loopSubframes) * channels;
    if (repetitions == 255) totalBlocks = 150;
    std::array<std::uint32_t, 2> command{0, 1};
    for (auto consumed = 0u; consumed < totalBlocks; ++consumed) Submit(channel, 18, command.data());
    if (repetitions == 255) {
        Require(((words[0] >> 12) & 255) == 255 && (words[0] & (1u << 20)) != 0, "Infinite XMA loop exhausted its input or repetitions");
        Require((words[1] >> 31) == 0, "Infinite XMA loop stopped producing PCM");
    } else {
        Require(((words[0] >> 12) & 255) == 0, "Finite XMA loop did not consume all repetitions");
        Require((words[0] & (3u << 20)) == 0 && (words[1] >> 31) == 1, "Finite XMA loop did not drain");
        Require((words[0] >> 27) == (words[9] & 31), "Finite XMA loop produced an incorrect duration");
        ExpectException<std::out_of_range>([&] { channel.SubmitCommand(18, 0, ~0u, command.data(), 0); });
    }
    auto* samples = static_cast<const std::byte*>(output);
    Require(std::all_of(samples, samples + 5888, [](auto value) { return value == std::byte{}; }), "Looping XMA silence produced nonzero PCM");
    Submit(channel, 7, &index);
    ApuFree(input);
    ApuFree(output);
    ApuFree(work);
}
}

void CheckXmaLoops(IAcpHal& channel, AcpHalShapeContexts& descriptor) {
    for (auto channels : {1u, 2u}) {
        for (auto start : {0u, 1u, 2u, 3u, 4u}) {
            for (auto end : {0u, 1u, 2u, 3u}) Loop(channel, descriptor, channels, 2, start, end, false);
        }
        for (auto count : {1u, 7u, 254u, 255u}) Loop(channel, descriptor, channels, count, 1, 2, true);
    }
}
