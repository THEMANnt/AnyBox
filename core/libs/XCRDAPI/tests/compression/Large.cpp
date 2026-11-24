#include "Tests.hpp"
#include "TemporaryFile.hpp"

#include <algorithm>
#include <array>

void AnyBox::Tests::CheckLargeCompressedFile() {
    std::vector<std::byte> plain(24 * 1024 * 1024);
    std::uint32_t random = 0x7A193B57;
    for (std::size_t offset = 0; offset < plain.size(); offset += 256) {
        for (std::size_t byte = 0; byte < 192; ++byte) {
            random ^= random << 13;
            random ^= random >> 17;
            random ^= random << 5;
            plain[offset + byte] = std::byte(random);
        }
        std::copy_n(plain.begin() + offset, 64, plain.begin() + offset + 192);
    }
    std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(libdeflate_alloc_compressor(6), libdeflate_free_compressor);
    Require(compressor != nullptr, "Large stream compressor allocation failed");
    std::vector<std::byte> packed(libdeflate_zlib_compress_bound(compressor.get(), plain.size()));
    auto size = libdeflate_zlib_compress(compressor.get(), plain.data(), plain.size(), packed.data(), packed.size());
    Require(size > 16 * 1024 * 1024 && size < plain.size(), "Large stream does not exceed the configured staging buffer");
    packed.resize(size);
    CompressedFile input(packed);
    auto factory = GetFactory();
    factory->SetStagingBufferSize(0);
    factory->SetStagingBufferSize(16 * 1024 * 1024);
    StorageStatistics baseline{};
    factory->GetStats(&baseline);
    {
        std::vector<std::byte> output(plain.size() + 2, std::byte{0x59});
        QueueFixture fixture(0);
        void* object = nullptr;
        factory->OpenFile(input.Path(), Storage::FileId, &object);
        Object<IStorageFile> file(static_cast<IStorageFile*>(object));
        StorageRequest request{};
        request.options = 1;
        request.source = file.get();
        request.sourceOffset = 16;
        request.sourceSize = static_cast<std::uint32_t>(packed.size());
        request.destination = output.data() + 1;
        request.destinationSize = static_cast<std::uint32_t>(plain.size());
        fixture.queue->EnqueueRequest(&request);
        fixture.Wait();
        Require(fixture.status->GetHResult(0) == 0, "File stream larger than staging failed");
        Require(std::equal(plain.begin(), plain.end(), output.begin() + 1), "Large file stream changed decompressed data");
        Require(output.front() == std::byte{0x59} && output.back() == std::byte{0x59}, "Large file stream overwrote its destination");
        StorageStatistics after{};
        factory->GetStats(&after);
        Require(after.totalStorageRequestCount > baseline.totalStorageRequestCount + 1, "Large file stream was not read incrementally");
        Require(after.totalDecompressionRequestCount == baseline.totalDecompressionRequestCount + 1 && after.decompressionErrors == baseline.decompressionErrors, "Large file stream corrupted decompression statistics");
        Require(after.maximumMemoryUsage == baseline.maximumMemoryUsage, "Large file stream allocated a request-sized staging buffer");
        request.sourceOffset = 17;
        ExpectException<std::invalid_argument>([&] { fixture.queue->EnqueueRequest(&request); });
    }
    StorageStatistics released{};
    factory->GetStats(&released);
    Require(released.currentMemoryUsage == baseline.currentMemoryUsage, "Large file stream retained resources after queue destruction");
    factory->SetStagingBufferSize(32 * 1024 * 1024);
}
