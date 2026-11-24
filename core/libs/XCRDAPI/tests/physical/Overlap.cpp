#include "Tests.hpp"

#include <algorithm>
#include <array>

void AnyBox::Tests::CheckPhysicalOverlaps() {
    {
        PhysicalBuffer buffer(2);
        auto frames = buffer.GetFrames();
        std::array<std::uint64_t, 2> source{frames[0], frames[1]};
        std::array<std::uint64_t, 2> destination{frames[1], frames[0]};
        auto memory = buffer.GetMemory();
        std::fill_n(memory.begin(), AnyBox::PhysicalPageSize, std::byte{0x17});
        std::fill(memory.begin() + AnyBox::PhysicalPageSize, memory.end(), std::byte{0x37});
        QueueFixture fixture(1);
        StorageRequest request{};
        request.options = (1ULL << 21) | (1ULL << 22) | (1ULL << 23);
        request.source = source.data();
        request.sourceSize = static_cast<std::uint32_t>(memory.size());
        request.destination = destination.data();
        request.destinationSize = request.sourceSize;
        fixture.queue->EnqueueRequest(&request);
        fixture.Wait();
        Require(fixture.status->GetHResult(0) == 0, "Overlapping physical copy failed");
        Require(std::all_of(memory.begin(), memory.begin() + AnyBox::PhysicalPageSize, [](auto value) { return value == std::byte{0x37}; }), "Physical copy overwrote an unread source page");
        Require(std::all_of(memory.begin() + AnyBox::PhysicalPageSize, memory.end(), [](auto value) { return value == std::byte{0x17}; }), "Physical copy lost the first source page");
    }
    PhysicalPacket data;
    for (bool physicalSource : {false, true}) {
        PhysicalBuffer buffer(3);
        auto frames = buffer.GetFrames();
        std::array<std::uint64_t, 3> destination{frames[2], frames[0], frames[1]};
        std::copy(data.packed.begin(), data.packed.end(), buffer.GetMemory().begin());
        QueueFixture fixture(1);
        StorageRequest request{};
        request.options = 1 | (1ULL << 21) | (1ULL << 22) | (physicalSource ? 1ULL << 23 : 0);
        request.source = physicalSource ? const_cast<std::uint64_t*>(frames.data()) : static_cast<void*>(buffer.GetMemory().data());
        request.sourceSize = static_cast<std::uint32_t>(data.packed.size());
        request.destination = destination.data();
        request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
        fixture.queue->EnqueueRequest(&request);
        fixture.Wait();
        Require(fixture.status->GetHResult(0) == 0, "In-place physical decompression failed");
        buffer.Map(destination);
        Require(std::equal(data.plain.begin(), data.plain.end(), buffer.GetMemory().begin()), "In-place physical decompression lost unread compressed data");
    }
}
