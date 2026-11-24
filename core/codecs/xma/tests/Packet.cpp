#include <anybox/codecs/XmaFrameAssembler.hpp>
#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using Packet = std::array<std::byte, 2048>;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

bool Bit(std::span<const std::byte> data, std::uint32_t offset) {
    return ((std::to_integer<unsigned>(data[offset / 8]) >> (7 - offset % 8)) & 1) != 0;
}

void Set(std::span<std::byte> data, std::uint32_t offset, bool value) {
    auto mask = static_cast<std::byte>(1u << (7 - offset % 8));
    if (value) data[offset / 8] |= mask;
    else data[offset / 8] &= ~mask;
}

std::vector<std::byte> Frame(std::uint32_t length, bool more = false) {
    std::vector<std::byte> frame((length + 7) / 8);
    for (std::uint32_t bit = 0; bit < length; ++bit) Set(frame, bit, (bit % 11) < 5);
    for (std::uint32_t bit = 0; bit < 15; ++bit) Set(frame, bit, ((length >> (14 - bit)) & 1) != 0);
    Set(frame, length - 1, more);
    return frame;
}

void Header(Packet& packet, std::uint32_t offset, std::uint32_t count) {
    auto value = offset - 32;
    packet[0] = static_cast<std::byte>((count << 2) | (value >> 13));
    packet[1] = static_cast<std::byte>(value >> 5);
    packet[2] = static_cast<std::byte>((value << 3) | 5);
    packet[3] = std::byte{2};
}

void CheckFrame(const AnyBox::Codecs::XmaPacketResult& result, std::span<const std::byte> expected, std::uint32_t length) {
    Require(result.frameBitCount == length, "XMA frame bit count changed");
    for (std::uint32_t bit = 0; bit < length; ++bit) Require(Bit(result.frame, bit) == Bit(expected, bit), "XMA assembled frame bits changed");
}

void CheckSplit(std::uint32_t offset, std::uint32_t length) {
    AnyBox::Codecs::XmaFrameAssembler assembler;
    auto frame = Frame(length);
    auto remaining = length;
    auto copied = 0u;
    auto first = true;
    while (remaining != 0) {
        Packet packet{};
        Header(packet, first ? offset : 32, first ? 1 : 0);
        auto start = first ? offset : 32;
        auto count = std::min(remaining, 16384 - start);
        for (std::uint32_t bit = 0; bit < count; ++bit) Set(packet, start + bit, Bit(frame, copied + bit));
        auto result = assembler.Read(packet, 0);
        remaining -= count;
        copied += count;
        Require(result.metadata == 5 && result.packetDistance == 3 && result.nextBitOffset == 0, "XMA packet navigation changed");
        if (remaining == 0) CheckFrame(result, frame, length);
        else Require(result.frame.empty() && assembler.HasPartialFrame(), "XMA split frame was not retained");
        first = false;
    }
    Require(!assembler.HasPartialFrame(), "Completed XMA frame retained partial state");
}

void Run() {
    for (auto offset : {32u, 33u, 34u, 35u, 36u, 37u, 38u, 39u, 16370u, 16377u, 16380u, 16383u}) {
        for (auto length : {16u, 23u, 511u, 16349u, 32766u}) CheckSplit(offset, length);
    }
    AnyBox::Codecs::XmaFrameAssembler assembler;
    Packet packet{};
    Header(packet, 40, 2);
    auto first = Frame(23, true);
    auto second = Frame(28);
    for (auto bit = 0u; bit < 23; ++bit) Set(packet, 40 + bit, Bit(first, bit));
    for (auto bit = 0u; bit < 28; ++bit) Set(packet, 63 + bit, Bit(second, bit));
    auto result = assembler.Read(packet, 0);
    CheckFrame(result, first, 23);
    Require(result.nextBitOffset == 63 && result.packetDistance == 0, "XMA next frame offset changed");
    CheckFrame(assembler.Read(packet, result.nextBitOffset), second, 28);

    packet.fill(std::byte{});
    Header(packet, 32, 0);
    Require(assembler.Read(packet, 0).frame.empty(), "Empty XMA packet produced a frame");
    Header(packet, 32, 1);
    bool rejected = false;
    try { assembler.Read(packet, 0); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "XMA frame with a zero length was accepted");
    assembler.Reset();
    auto sentinel = Frame(32767);
    for (auto bit = 0u; bit < 15; ++bit) Set(packet, 32 + bit, Bit(sentinel, bit));
    Require(assembler.Read(packet, 0).frame.empty() && !assembler.HasPartialFrame(), "XMA packet terminator produced a frame");
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
