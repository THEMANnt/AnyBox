#include "Tests.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <exception>
#include <thread>

namespace {

class TemporaryFile {
public:
    TemporaryFile() {
        std::array<wchar_t, MAX_PATH> directory;
        AnyBox::Tests::Require(GetTempPathW(static_cast<DWORD>(directory.size()), directory.data()) != 0, "Temporary directory query failed");
        AnyBox::Tests::Require(GetTempFileNameW(directory.data(), L"abq", 0, path.data()) != 0, "Temporary file creation failed");
        AnyBox::Storage::Handle handle(CreateFileW(path.data(), GENERIC_WRITE, 0, nullptr, TRUNCATE_EXISTING, 0, nullptr));
        if (handle.Get() == INVALID_HANDLE_VALUE) AnyBox::Windows::ThrowLastError("CreateFileW queue test");
        std::array<std::byte, 4096> data;
        for (std::size_t i = 0; i < data.size(); ++i) data[i] = std::byte(i % 251);
        DWORD written = 0;
        AnyBox::Tests::Require(WriteFile(handle.Get(), data.data(), static_cast<DWORD>(data.size()), &written, nullptr) != 0 && written == data.size(), "Queue test file write failed");
    }
    ~TemporaryFile() { DeleteFileW(path.data()); }
    const wchar_t* Path() const { return path.data(); }

private:
    std::array<wchar_t, MAX_PATH> path;
};

}

void AnyBox::Tests::CheckFileQueue() {
    TemporaryFile source;
    std::array<std::byte, 257> buffer{};
    QueueFixture test(0);
    void* output = nullptr;
    test.factory->OpenFile(source.Path(), Storage::FileId, &output);
    Object<IStorageFile> file(static_cast<IStorageFile*>(output));
    StorageRequest request{};
    request.source = file.get();
    request.sourceOffset = 197;
    request.sourceSize = static_cast<std::uint32_t>(buffer.size());
    request.destination = buffer.data();
    request.destinationSize = request.sourceSize;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "Queued file read failed");
    for (std::size_t i = 0; i < buffer.size(); ++i) Require(buffer[i] == std::byte((197 + i) % 251), "Queued file read returned incorrect bytes");
    request.sourceOffset = 4096;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == std::bit_cast<std::int32_t>(0x80070000U | ERROR_HANDLE_EOF), "Queue did not propagate the short read error");
    Storage::Handle error(test.queue->GetErrorEvent());
    Require(WaitForSingleObject(error.Get(), 0) == WAIT_OBJECT_0, "Queue error event was not set");
    CheckFileErrorRecord(test.queue.get(), request, source.Path());
    request.sourceOffset = 0;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "Queue reused an error from a previous status group");
    test.queue->EnqueueRequest(&request);
    Storage::Handle started(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    Storage::Handle closed(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    Require(started.Get() && closed.Get(), "File close test event creation failed");
    std::exception_ptr closeError;
    std::jthread closer([&] {
        SetEvent(started.Get());
        try {
            file->Close();
        } catch (...) {
            closeError = std::current_exception();
        }
        SetEvent(closed.Get());
    });
    auto startedResult = WaitForSingleObject(started.Get(), 5000);
    auto beforeSubmit = WaitForSingleObject(closed.Get(), 50);
    test.Wait();
    auto finished = WaitForSingleObject(closed.Get(), 5000);
    closer.join();
    if (closeError) std::rethrow_exception(closeError);
    Require(startedResult == WAIT_OBJECT_0 && beforeSubmit == WAIT_TIMEOUT, "DirectStorage Close returned before pending IO completed");
    Require(finished == WAIT_OBJECT_0, "DirectStorage Close did not observe completed IO");
    ExpectException<std::logic_error>([&] { file->GetHandle(); });
    file.reset();
    test.factory->OpenFile(source.Path(), Storage::FileId, &output);
    file.reset(static_cast<IStorageFile*>(output));
    request.source = file.get();
    Require(ResetEvent(started.Get()) && ResetEvent(closed.Get()), "File release test event reset failed");
    test.queue->EnqueueRequest(&request);
    auto* lastReference = file.release();
    std::uint32_t releaseCount = 99;
    std::jthread releaser([&] {
        SetEvent(started.Get());
        releaseCount = lastReference->Release();
        SetEvent(closed.Get());
    });
    startedResult = WaitForSingleObject(started.Get(), 5000);
    beforeSubmit = WaitForSingleObject(closed.Get(), 50);
    test.Wait();
    finished = WaitForSingleObject(closed.Get(), 5000);
    releaser.join();
    Require(startedResult == WAIT_OBJECT_0 && beforeSubmit == WAIT_TIMEOUT, "Last file Release returned before pending IO completed");
    Require(finished == WAIT_OBJECT_0 && releaseCount == 0, "Last file Release did not retire completed IO");
    test.queue->Close();
}
