#include "../memory/Tests.hpp"
#include "XmaPacket.hpp"
#include <IAcpHal.hpp>
#include <algorithm>
#include <cstring>

namespace {
void Submit(IAcpHal& channel, std::uint32_t type, const void* data) {
    AnyBox::Tests::Require(channel.SubmitCommand(type, 0, ~0u, data, 0) == 0, "SHAPE buffer command failed");
    AcpHalMessage message{};
    AnyBox::Tests::Require(channel.PopMessage(&message), "SHAPE buffer completion was lost");
}

void Split(IAcpHal& channel, AcpHalShapeContexts& descriptor, std::uint32_t channels, bool delayed) {
    using namespace AnyBox::Tests;
    void* input0 = nullptr;
    void* input1 = nullptr;
    void* output = nullptr;
    void* work = nullptr;
    std::uint32_t inputBus0 = 0, inputBus1 = 0, outputBus = 0, workBus = 0;
    ApuAlloc(&input0, &inputBus0, 2048, 2048, 1);
    ApuAlloc(&input1, &inputBus1, 2048, 2048, 1);
    ApuAlloc(&output, &outputBus, 5888, 256, 2);
    ApuAlloc(&work, &workBus, 512, 256, 2);
    auto frame = MakeXmaSilence(channels, 1);
    auto frameSize = channels == 1 ? 31u : 36u;
    std::array<std::byte, 2048> first{}, second{};
    constexpr auto offset = 16376u;
    auto headerOffset = offset - 32;
    first[0] = static_cast<std::byte>(4 | (headerOffset >> 13));
    first[1] = static_cast<std::byte>(headerOffset >> 5);
    first[2] = static_cast<std::byte>(headerOffset << 3);
    for (auto bit = 0u; bit < frameSize; ++bit) {
        auto value = ((std::to_integer<unsigned>(frame[(32 + bit) / 8]) >> (7 - (32 + bit) % 8)) & 1) != 0;
        if (bit < 8) SetXmaBit(first, offset + bit, value);
        else SetXmaBit(second, 32 + bit - 8, value);
    }
    std::memcpy(input0, first.data(), 2048);
    std::memcpy(input1, second.data(), 2048);
    std::memset(output, 0x73, 5888);
    std::memset(work, 0, 512);
    auto* words = static_cast<std::uint32_t*>(descriptor.contexts[4]);
    std::fill_n(words, 16, 0u);
    words[0] = 1 | (1 << 20) | (23 << 22) | (delayed ? 0 : (1 << 21));
    words[1] = 1 | (4 << 20) | ((channels - 1) << 29) | (1u << 31);
    words[5] = inputBus0;
    words[6] = inputBus1;
    words[7] = outputBus;
    words[8] = workBus;
    std::uint32_t index = 0;
    Submit(channel, 5, &index);
    if (delayed) {
        Require((words[0] >> 27) == 0 && (words[0] & (1u << 20)) == 0, "Partial XMA frame produced PCM or retained exhausted input");
        words[0] |= 1 << 21;
        Submit(channel, 5, &index);
    }
    auto blocks = 4 * channels;
    Require((words[0] >> 27) == blocks, "Split XMA frame produced an incorrect number of PCM blocks");
    Require((words[0] & (3u << 20)) == 0, "Split XMA frame retained consumed input buffers");
    auto* bytes = static_cast<const std::byte*>(output);
    Require(std::all_of(bytes, bytes + blocks * 256, [](auto value) { return value == std::byte{}; }), "Split XMA silence decoded incorrectly");
    Require(bytes[blocks * 256] == std::byte{0x73}, "Split XMA frame overwrote unused output storage");
    std::array<std::uint32_t, 2> consumed{0, blocks};
    Submit(channel, 18, consumed.data());
    Submit(channel, 7, &index);
    ApuFree(input0);
    ApuFree(input1);
    ApuFree(output);
    ApuFree(work);
}
}

void CheckXmaBuffers(IAcpHal& channel, AcpHalShapeContexts& descriptor) {
    for (auto channels : {1u, 2u}) {
        for (auto delayed : {false, true}) Split(channel, descriptor, channels, delayed);
    }
}
