#include "Tests.hpp"

#include <algorithm>
#include <array>

void AnyBox::Tests::CheckDisjointSharedPage() {
    CompressedData data;
    std::vector<std::byte> warm(data.plain.size());
    PhysicalBuffer buffer(1);
    auto memory = buffer.GetMemory();
    Require(data.packed.size() + 64 < 8192 && data.plain.size() + 8192 < memory.size(), "Shared page test packet does not fit its disjoint ranges");
    std::fill(memory.begin(), memory.end(), std::byte{0x57});
    std::copy(data.packed.begin(), data.packed.end(), memory.begin() + 64);
    QueueFixture fixture(1);
    StorageRequest request{};
    request.options = 1 | (1ULL << 22);
    request.source = data.packed.data();
    request.sourceSize = static_cast<std::uint32_t>(data.packed.size());
    request.destination = warm.data();
    request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(fixture.status->GetHResult(0) == 0 && warm == data.plain, "Shared page decoder warmup failed");

    StorageStatistics before{};
    fixture.factory->GetStats(&before);
    auto frames = buffer.GetFrames();
    request.options |= (1ULL << 21) | (1ULL << 23);
    request.source = const_cast<std::uint64_t*>(frames.data());
    request.sourceOffset = 64;
    request.destination = request.source;
    request.destinationPageOffset = 8192;
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(fixture.status->GetHResult(0) == 0, "Disjoint ranges in a shared physical page failed to decompress");
    Require(std::equal(data.plain.begin(), data.plain.end(), memory.begin() + 8192), "Shared physical page decompression changed the result");
    StorageStatistics after{};
    fixture.factory->GetStats(&after);
    Require(after.currentMemoryUsage == before.currentMemoryUsage, "Disjoint physical ranges unnecessarily captured compressed input");
    Require(memory[8191] == std::byte{0x57} && memory[8192 + data.plain.size()] == std::byte{0x57}, "Shared physical page decompression damaged neighboring ranges");
}
