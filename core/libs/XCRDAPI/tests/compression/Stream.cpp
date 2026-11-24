#include "Tests.hpp"
#include "../../src/compression/ZlibStream.hpp"
#include "../../src/metrics/Counters.hpp"

#include <algorithm>
#include <array>

namespace {

void Decode(AnyBox::Storage::ZlibStream& stream, std::span<const std::byte> input, std::span<std::byte> output, std::size_t chunk) {
    stream.Reset(output);
    for (std::size_t position = 0; position < input.size();) {
        auto size = std::min(chunk, input.size() - position);
        stream.Process(input.subspan(position, size), position + size == input.size());
        position += size;
    }
}

}

void AnyBox::Tests::CheckZlibStream() {
    CompressedData data;
    std::vector<std::byte> output(data.plain.size() + 2, std::byte{0x57});
    auto destination = std::span(output).subspan(1, data.plain.size());
    Storage::ZlibStream stream;
    ExpectException<std::logic_error>([&] { stream.Process(data.packed, true); });
    for (auto chunk : {1U, 2U, 7U, 64U, 131072U}) {
        Decode(stream, data.packed, destination, chunk);
        Require(std::equal(data.plain.begin(), data.plain.end(), destination.begin()), "Streaming decoder changed the output");
        Require(output.front() == std::byte{0x57} && output.back() == std::byte{0x57}, "Streaming decoder overwrote its destination");
    }
    ExpectException<std::logic_error>([&] { stream.Process(data.packed, true); });
    auto truncated = std::span(data.packed).first(data.packed.size() - 1);
    ExpectException<std::runtime_error>([&] { Decode(stream, truncated, destination, 7); });
    auto corrupted = data.packed;
    corrupted.back() ^= std::byte{1};
    ExpectException<std::runtime_error>([&] { Decode(stream, corrupted, destination, 64); });
    auto trailing = data.packed;
    trailing.push_back(std::byte{0x71});
    ExpectException<std::runtime_error>([&] { Decode(stream, trailing, destination, 1); });
    ExpectException<std::runtime_error>([&] { Decode(stream, trailing, destination, trailing.size()); });
    ExpectException<std::length_error>([&] { Decode(stream, data.packed, destination.first(destination.size() - 1), 7); });
    std::vector<std::byte> oversized(data.plain.size() + 1);
    ExpectException<std::length_error>([&] { Decode(stream, data.packed, oversized, 7); });
    ExpectException<std::length_error>([&] { stream.Reset({}); });
    stream.Reset(destination);
    ExpectException<std::runtime_error>([&] { stream.Process({}, true); });
    Decode(stream, data.packed, destination, 7);
    Require(std::equal(data.plain.begin(), data.plain.end(), destination.begin()), "Streaming decoder did not recover after an invalid stream");
    auto warmMemory = Storage::GetCounters().currentMemory.load();
    for (std::uint32_t repeat = 0; repeat < 32; ++repeat) Decode(stream, data.packed, destination, 7);
    Require(Storage::GetCounters().currentMemory.load() == warmMemory, "Streaming decoder allocated new workspaces between requests");
    Require(output.front() == std::byte{0x57} && output.back() == std::byte{0x57}, "Invalid stream damaged destination guards");
}
