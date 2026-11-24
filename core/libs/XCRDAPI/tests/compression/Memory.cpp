#include "Tests.hpp"

#include <algorithm>
#include <bit>

void AnyBox::Tests::CheckCompressedMemory() {
    CompressedData data;
    std::vector<std::byte> output(data.plain.size() + 2, std::byte{0x39});
    auto factory = GetFactory();
    factory->SetStagingBufferSize(0);
    {
        QueueFixture test(1);
        StorageRequest request{};
        request.options = (1ULL << 22) | 1;
        request.source = data.packed.data();
        request.sourceSize = static_cast<std::uint32_t>(data.packed.size());
        request.destination = output.data() + 1;
        request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
        test.queue->EnqueueRequest(&request);
        test.Wait();
        Require(test.status->GetHResult(0) == 0, "Memory-sourced zlib decompression failed");
        Require(std::equal(data.plain.begin(), data.plain.end(), output.begin() + 1), "Memory-sourced zlib payload changed");
        Require(output.front() == std::byte{0x39} && output.back() == std::byte{0x39}, "Memory-sourced zlib output overran its destination");

        std::copy(data.packed.begin(), data.packed.end(), output.begin() + 1);
        request.source = output.data() + 1;
        test.queue->EnqueueRequest(&request);
        test.Wait();
        Require(test.status->GetHResult(0) == 0, "In-place zlib decompression failed with staging disabled");
        Require(std::equal(data.plain.begin(), data.plain.end(), output.begin() + 1), "In-place zlib decompression changed its payload");

        request.source = data.packed.data();
        data.packed.back() ^= std::byte{1};
        test.queue->EnqueueRequest(&request);
        test.Wait();
        Require(test.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_INVALID_DATA), "Corrupted zlib data did not fail its status group");
        data.packed.back() ^= std::byte{1};
        request.destinationSize = request.sourceSize - 1;
        ExpectException<std::invalid_argument>([&] { test.queue->EnqueueRequest(&request); });
        test.queue->Close();
    }
    factory->SetStagingBufferSize(32 * 1024 * 1024);
}
