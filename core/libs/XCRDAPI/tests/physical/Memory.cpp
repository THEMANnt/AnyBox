#include "Tests.hpp"

#include <algorithm>
#include <array>

namespace {

void CheckMemoryCase(bool compressed, bool sourcePhysical, bool destinationPhysical, bool scattered) {
    using namespace AnyBox::Tests;
    PhysicalPacket data;
    const auto& payload = compressed ? data.packed : data.plain;
    PhysicalBuffer source(3);
    PhysicalBuffer destination(3);
    auto sourceFrames = source.GetFrames();
    auto destinationFrames = destination.GetFrames();
    std::array<std::uint64_t, 3> inputOrder{sourceFrames[0], sourceFrames[1], sourceFrames[2]};
    std::array<std::uint64_t, 3> outputOrder{destinationFrames[0], destinationFrames[1], destinationFrames[2]};
    if (scattered) {
        std::swap(inputOrder[0], inputOrder[2]);
        std::swap(outputOrder[0], outputOrder[1]);
    }
    source.Map(inputOrder);
    destination.Map(outputOrder);
    constexpr std::uint16_t inputOffset = 65517;
    constexpr std::uint16_t outputOffset = 65511;
    std::vector<std::byte> input(payload.size() + 2, std::byte{0x37});
    std::vector<std::byte> output(data.plain.size() + 2, std::byte{0x53});
    std::copy(payload.begin(), payload.end(), input.begin() + 1);
    std::fill(destination.GetMemory().begin(), destination.GetMemory().end(), std::byte{0x53});
    std::copy(payload.begin(), payload.end(), source.GetMemory().begin() + inputOffset);
    QueueFixture fixture(1);
    StorageRequest request{};
    request.options = (1ULL << 22) | static_cast<std::uint64_t>(compressed) | (sourcePhysical ? 1ULL << 23 : 0) | (destinationPhysical ? 1ULL << 21 : 0);
    request.source = sourcePhysical ? inputOrder.data() : static_cast<void*>(input.data() + 1);
    request.sourceOffset = sourcePhysical ? 0xA573000000000000ULL | inputOffset : 0xFFFFFFFFFFFFFFFFULL;
    request.sourceSize = static_cast<std::uint32_t>(payload.size());
    request.destination = destinationPhysical ? outputOrder.data() : static_cast<void*>(output.data() + 1);
    request.destinationPageOffset = destinationPhysical ? outputOffset : 0xFFFF;
    request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
    request.padding0.fill(0xA5);
    request.padding1 = 0xB713A857;
    StorageStatistics before{};
    fixture.factory->GetStats(&before);
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(fixture.status->GetHResult(0) == 0, "Physical memory request failed");
    if (!compressed) {
        StorageStatistics after{};
        fixture.factory->GetStats(&after);
        Require(after.currentMemoryUsage == before.currentMemoryUsage, "Disjoint physical copy allocated a data staging buffer");
    }
    auto actual = destinationPhysical ? destination.GetMemory().subspan(outputOffset, data.plain.size()) : std::span(output).subspan(1, data.plain.size());
    Require(std::equal(data.plain.begin(), data.plain.end(), actual.begin()), "Physical memory request changed its data or page order");
    if (destinationPhysical) {
        auto memory = destination.GetMemory();
        Require(std::all_of(memory.begin(), memory.begin() + outputOffset, [](auto value) { return value == std::byte{0x53}; }), "Physical request wrote before its page offset");
        Require(std::all_of(memory.begin() + outputOffset + data.plain.size(), memory.end(), [](auto value) { return value == std::byte{0x53}; }), "Physical request wrote past its destination size");
    } else Require(output.front() == std::byte{0x53} && output.back() == std::byte{0x53}, "Physical source request overwrote its destination");
}

}

void AnyBox::Tests::CheckPhysicalCopies() {
    for (bool source : {false, true}) for (bool destination : {false, true}) for (bool scattered : {false, true}) CheckMemoryCase(false, source, destination, scattered);
}

void AnyBox::Tests::CheckPhysicalMemoryDecode() {
    for (bool source : {false, true}) for (bool destination : {false, true}) for (bool scattered : {false, true}) CheckMemoryCase(true, source, destination, scattered);
}
