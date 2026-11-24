#include "Tests.hpp"

#include <algorithm>
#include <array>
#include <bit>

void AnyBox::Tests::CheckPhysicalCancellation() {
    std::array<std::byte, 257> source{};
    source.fill(std::byte{0x73});
    PhysicalBuffer destination(1);
    auto memory = destination.GetMemory();
    std::fill(memory.begin(), memory.end(), std::byte{0x57});
    destination.Unmap();
    auto frames = destination.GetFrames();
    QueueFixture fixture(1);
    StorageRequest request{};
    request.options = (1ULL << 21) | (1ULL << 22);
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destination = const_cast<std::uint64_t*>(frames.data());
    request.destinationSize = request.sourceSize;
    request.cancellationTag = 0x7357;
    fixture.queue->EnqueueRequest(&request);
    fixture.queue->CancelRequestsWithTag(0xFFFF, 0x7357);
    ExpectException<std::logic_error>([&] { destination.FreePages(); });
    fixture.Wait();
    Require(fixture.status->GetHResult(0) == 0, "Canceled physical request incorrectly failed its status group");
    destination.Map(frames);
    memory = destination.GetMemory();
    Require(std::all_of(memory.begin(), memory.end(), [](auto value) { return value == std::byte{0x57}; }), "Canceled physical request changed its destination");
    destination.Unmap();
    destination.FreePages();
}

void AnyBox::Tests::CheckPhysicalFailure() {
    PhysicalBuffer source(1);
    PhysicalBuffer destination(1);
    auto input = source.GetMemory();
    std::fill(input.begin(), input.end(), std::byte{0});
    source.Unmap();
    destination.Unmap();
    auto from = source.GetFrames();
    auto to = destination.GetFrames();
    QueueFixture fixture(1);
    StorageRequest request{};
    request.options = 1 | (1ULL << 21) | (1ULL << 22) | (1ULL << 23);
    request.source = const_cast<std::uint64_t*>(from.data());
    request.sourceSize = 16;
    request.destination = const_cast<std::uint64_t*>(to.data());
    request.destinationSize = 4096;
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    auto failure = std::bit_cast<std::int32_t>(0x80070000U | ERROR_INVALID_DATA);
    Require(fixture.status->GetHResult(0) == failure, "Invalid physical zlib stream did not report invalid data");
    source.FreePages();
    destination.FreePages();
    StorageErrorRecord record;
    fixture.queue->RetrieveErrorRecord(&record);
    Require(record.failureCount == 1 && record.firstFailure.result == failure, "Physical request failure record is missing");
    Require(record.firstFailure.request.options == request.options && record.firstFailure.request.source == request.source && record.firstFailure.request.destination == request.destination, "Physical failure record changed its original request parameters");
}
