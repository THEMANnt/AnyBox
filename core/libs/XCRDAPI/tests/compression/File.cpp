#include "Tests.hpp"
#include "TemporaryFile.hpp"

#include <algorithm>
#include <array>
#include <bit>

void AnyBox::Tests::CheckCompressedFile() {
    CompressedData data;
    CompressedFile source(data.packed);
    std::vector<std::byte> output(data.plain.size() + 2, std::byte{0x53});
    QueueFixture test(0);
    void* object = nullptr;
    test.factory->OpenFile(source.Path(), Storage::FileId, &object);
    Object<IStorageFile> file(static_cast<IStorageFile*>(object));
    StorageRequest request{};
    request.options = 1;
    request.source = file.get();
    request.sourceOffset = 16;
    request.sourceSize = static_cast<std::uint32_t>(data.packed.size());
    request.destination = output.data() + 1;
    request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "File-sourced zlib decompression failed");
    Require(std::equal(data.plain.begin(), data.plain.end(), output.begin() + 1), "File-sourced zlib data changed");
    Require(output.front() == std::byte{0x53} && output.back() == std::byte{0x53}, "File-sourced zlib output overran its destination");
    StorageStatistics before{};
    test.factory->GetStats(&before);
    request.sourceOffset = (data.packed.size() + 47) & ~std::uint64_t{15};
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_HANDLE_EOF), "Truncated file IO did not report EOF");
    StorageStatistics failedRead{};
    test.factory->GetStats(&failedRead);
    Require(failedRead.storageErrors == before.storageErrors + 1 && failedRead.decompressionErrors == before.decompressionErrors, "File IO error was counted as a decompression error");

    request.sourceOffset = 16;
    --request.sourceSize;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_INVALID_DATA), "Truncated zlib stream did not report invalid data");
    StorageStatistics failedDecode{};
    test.factory->GetStats(&failedDecode);
    Require(failedDecode.storageErrors == failedRead.storageErrors && failedDecode.decompressionErrors == failedRead.decompressionErrors + 1, "Decoder error was counted as a file IO error");

    ++request.sourceSize;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "File decoder did not recover after IO and data errors");
    Require(std::equal(data.plain.begin(), data.plain.end(), output.begin() + 1), "Recovered file decoder changed the output");
    Require(output.front() == std::byte{0x53} && output.back() == std::byte{0x53}, "Failed file decoder overwrote its destination");
    test.queue->Close();
}
