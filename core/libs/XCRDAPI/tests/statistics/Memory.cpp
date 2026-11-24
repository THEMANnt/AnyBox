#include "Tests.hpp"

#include <array>
#include <bit>

void AnyBox::Tests::CheckStatisticsMemory() {
    QueueFixture fixture(1);
    auto before = ReadStatistics(fixture.factory.get());
    std::array<std::byte, 13> source{};
    source.fill(std::byte{0x73});
    std::array<std::byte, 13> destination{};
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destination = destination.data();
    request.destinationSize = request.sourceSize;
    fixture.queue->EnqueueRequest(&request);
    auto unpublished = ReadStatistics(fixture.factory.get());
    Require(unpublished.priorities[3].totalRequestCount == before.priorities[3].totalRequestCount, "Unpublished request was counted as submitted");
    Require(unpublished.priorities[3].pendingRequestCount == 0, "Unpublished request was counted as pending");
    fixture.Wait();
    Require(destination == source, "Statistics memory request failed");
    auto raw = ReadStatistics(fixture.factory.get());
    Require(raw.priorities[3].totalRequestCount == before.priorities[3].totalRequestCount + 1, "Submitted request was not counted");
    Require(raw.priorities[3].totalSourceBytes == before.priorities[3].totalSourceBytes + source.size(), "Memory source bytes were counted incorrectly");
    Require(raw.priorities[3].totalDestinationBytes == before.priorities[3].totalDestinationBytes + destination.size(), "Memory destination bytes were counted incorrectly");
    Require(raw.totalStorageRequestCount == before.totalStorageRequestCount, "Memory request was counted as file IO");

    CompressedData compressed;
    std::vector<std::byte> decoded(compressed.plain.size());
    request.options |= 1;
    request.source = compressed.packed.data();
    request.sourceSize = static_cast<std::uint32_t>(compressed.packed.size());
    request.destination = decoded.data();
    request.destinationSize = static_cast<std::uint32_t>(decoded.size());
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(decoded == compressed.plain, "Statistics decompression request failed");
    auto unpacked = ReadStatistics(fixture.factory.get());
    Require(unpacked.totalDecompressionRequestCount == before.totalDecompressionRequestCount + 1, "Decompression was not counted");
    Require(unpacked.pendingDecompressionRequestCount == 0 && unpacked.priorities[3].pendingRequestCount == 0, "Completed decompression remained pending");
    Require(unpacked.currentMemoryUsage > before.currentMemoryUsage, "Codec workspace was not included in public statistics");

    std::array<std::byte, 16> invalid{};
    request.source = invalid.data();
    request.sourceSize = static_cast<std::uint32_t>(invalid.size());
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    auto failed = ReadStatistics(fixture.factory.get());
    Require(fixture.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_INVALID_DATA), "Invalid compressed request unexpectedly succeeded");
    Require(failed.totalDecompressionRequestCount == before.totalDecompressionRequestCount + 2, "Failed decompression was not counted");
    Require(failed.decompressionErrors == before.decompressionErrors + 1 && failed.errorCount == before.errorCount + 1, "Decompression failure was not counted exactly once");
    Require(failed.storageErrors == before.storageErrors && failed.pendingDecompressionRequestCount == 0, "Decompression failure corrupted IO or pending statistics");
    StorageErrorRecord error;
    fixture.queue->RetrieveErrorRecord(&error);
    auto retrieved = ReadStatistics(fixture.factory.get());
    Require(retrieved.errorCount == failed.errorCount, "Error retrieval erased the cumulative error count");
    ExpectException<std::invalid_argument>([&] { fixture.queue->EnqueueRequest(nullptr); });
    auto rejected = ReadStatistics(fixture.factory.get());
    Require(rejected.errorCount == failed.errorCount + 1, "Rejected request was not counted as an error");
    Require(rejected.priorities[3].totalRequestCount == before.priorities[3].totalRequestCount + 3, "Rejected request was counted as submitted");
    Require(rejected.priorities[3].totalStatusCount == before.priorities[3].totalStatusCount + 3, "Status writes were not counted");
}
