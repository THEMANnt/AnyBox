#include "Tests.hpp"

#include <array>
#include <bit>

namespace {

class TemporaryFile {
public:
    explicit TemporaryFile(std::span<const std::byte> data) {
        std::array<wchar_t, MAX_PATH> directory{};
        AnyBox::Tests::Require(GetTempPathW(static_cast<DWORD>(directory.size()), directory.data()) != 0, "Statistics temporary directory query failed");
        AnyBox::Tests::Require(GetTempFileNameW(directory.data(), L"abs", 0, path.data()) != 0, "Statistics temporary file creation failed");
        AnyBox::Storage::Handle file(CreateFileW(path.data(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, 0, nullptr));
        if (file.Get() == INVALID_HANDLE_VALUE) AnyBox::Windows::ThrowLastError("CreateFileW statistics test");
        DWORD written = 0;
        AnyBox::Tests::Require(WriteFile(file.Get(), data.data(), static_cast<DWORD>(data.size()), &written, nullptr) && written == data.size(), "Statistics file write failed");
    }
    ~TemporaryFile() { DeleteFileW(path.data()); }
    const wchar_t* Path() const { return path.data(); }

private:
    std::array<wchar_t, MAX_PATH> path{};
};

}

void AnyBox::Tests::CheckStatisticsFile() {
    CompressedData compressed;
    TemporaryFile input(compressed.packed);
    QueueFixture fixture(0);
    auto baseline = ReadStatistics(fixture.factory.get());
    void* output = nullptr;
    fixture.factory->OpenFile(input.Path(), Storage::FileId, &output);
    Object<IStorageFile> file(static_cast<IStorageFile*>(output));
    auto opened = ReadStatistics(fixture.factory.get());
    Require(opened.currentFileCount == baseline.currentFileCount + 1 && opened.cumulativeFileCount == baseline.cumulativeFileCount + 1, "Opened file was not counted");
    Require(opened.maximumFileCount >= opened.currentFileCount, "File peak count is below the current count");
    Require(opened.totalStorageRequestCount == baseline.totalStorageRequestCount, "Win32 IO outside DirectStorage changed its request count");
    std::vector<std::byte> decoded(compressed.plain.size());
    StorageRequest request{};
    request.options = 1;
    request.source = file.get();
    request.sourceSize = static_cast<std::uint32_t>(compressed.packed.size());
    request.destination = decoded.data();
    request.destinationSize = static_cast<std::uint32_t>(decoded.size());
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(decoded == compressed.plain, "Statistics compressed file read failed");
    auto completed = ReadStatistics(fixture.factory.get());
    Require(completed.totalStorageRequestCount == baseline.totalStorageRequestCount + 1, "File read was not counted");
    Require(completed.totalDecompressionRequestCount == baseline.totalDecompressionRequestCount + 1, "File decompression was not counted");
    Require(completed.pendingStorageRequestCount == 0 && completed.pendingDecompressionRequestCount == 0, "Completed file read remained pending");
    Require(completed.priorities[1].totalRequestCount == baseline.priorities[1].totalRequestCount + 1, "File request was counted in the wrong priority");

    request.options = 0;
    request.sourceOffset = compressed.packed.size();
    request.destinationSize = request.sourceSize;
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    auto failed = ReadStatistics(fixture.factory.get());
    Require(fixture.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_HANDLE_EOF), "Short statistics read unexpectedly succeeded");
    Require(failed.totalStorageRequestCount == baseline.totalStorageRequestCount + 2, "Failed file read was not counted");
    Require(failed.storageErrors == baseline.storageErrors + 1 && failed.errorCount == baseline.errorCount + 1, "File error was not counted exactly once");
    Require(failed.totalDecompressionRequestCount == completed.totalDecompressionRequestCount, "Uncompressed failure was counted as decompression");
    file->Close();
    auto closed = ReadStatistics(fixture.factory.get());
    Require(closed.currentFileCount == baseline.currentFileCount, "Closed file remained open in statistics");
    auto liveMemory = closed.currentMemoryUsage;
    file.reset();
    auto released = ReadStatistics(fixture.factory.get());
    Require(released.currentMemoryUsage < liveMemory && released.cumulativeFileCount == opened.cumulativeFileCount, "File destruction did not release memory or lost history");
}
