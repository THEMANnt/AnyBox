#include "Tests.hpp"

#include <array>
#include <bit>
#include <string_view>

void AnyBox::Tests::CheckQueueErrors() {
    QueueFixture test(1);
    Storage::Handle event(test.queue->GetErrorEvent());
    struct GuardedRecord {
        StorageErrorRecord record;
        std::uint64_t guard = 0x173A8D473F168B21ULL;
    } output;
    test.queue->RetrieveErrorRecord(&output.record);
    Require(output.record.failureCount == 0 && output.record.firstFailure.commandType == -1, "Empty error record reported a failure");

    std::array<std::byte, 16> source{};
    std::array<std::byte, 128> destination{};
    std::array<char, 96> name;
    name.fill('x');
    name.back() = '\0';
    StorageRequest request{};
    request.options = (1ULL << 22) | 1;
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destination = destination.data();
    request.destinationSize = static_cast<std::uint32_t>(destination.size());
    request.cancellationTag = 0x572;
    request.name = name.data();
    test.queue->EnqueueRequest(&request);
    name.fill('y');
    name.back() = '\0';
    request.cancellationTag = 0x573;
    test.queue->EnqueueRequest(&request);
    test.Wait();

    Require(WaitForSingleObject(event.Get(), 0) == WAIT_OBJECT_0, "Failed requests did not set the error event");
    test.queue->RetrieveErrorRecord(&output.record);
    Require(output.guard == 0x173A8D473F168B21ULL, "Error record overwrote the caller buffer");
    Require(output.record.failureCount == 2, "Error record lost failures before retrieval");
    const auto& failure = output.record.firstFailure;
    Require(failure.result == std::bit_cast<std::int32_t>(0x80070000U | ERROR_INVALID_DATA), "Error record returned the wrong HRESULT");
    Require(failure.commandType == 0, "Error record returned the wrong command type");
    Require(failure.request.cancellationTag == 0x572, "Later failure overwrote the first failure");
    Require(failure.request.options == request.options, "Error record changed request options");
    Require(failure.request.source == source.data() && failure.request.sourceSize == source.size(), "Error record lost memory source");
    Require(failure.request.destination == destination.data() && failure.request.destinationSize == destination.size(), "Error record lost destination");
    Require(failure.request.filename.front() == u'\0', "Memory request reported a filename");
    Require(std::string_view(failure.request.name.data()) == std::string(63, 'x'), "Request name was not retained or truncated safely");
    Require(WaitForSingleObject(event.Get(), 0) == WAIT_TIMEOUT, "Retrieving an error did not reset its event");
    test.queue->RetrieveErrorRecord(&output.record);
    Require(output.record.failureCount == 0 && output.record.firstFailure.commandType == -1, "Retrieved failures were not cleared");

    request.name = nullptr;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    test.queue->RetrieveErrorRecord(&output.record);
    Require(output.record.failureCount == 1 && output.record.firstFailure.request.cancellationTag == 0x573, "New failure did not start a fresh record");
    Require(output.record.firstFailure.request.name.front() == '\0', "New failure reused a previous request name");
    ExpectException<std::invalid_argument>([&] { test.queue->RetrieveErrorRecord(nullptr); });
    test.queue->Close();
    ExpectException<std::logic_error>([&] { test.queue->RetrieveErrorRecord(&output.record); });
}

void AnyBox::Tests::CheckFileErrorRecord(IStorageQueue* queue, const StorageRequest& request, const wchar_t* path) {
    StorageErrorRecord record;
    queue->RetrieveErrorRecord(&record);
    Require(record.failureCount == 1 && record.firstFailure.commandType == 0, "File failure record is missing");
    const auto& output = record.firstFailure.request;
    Require(output.source == nullptr && output.fileOffset == request.sourceOffset, "File failure record lost its offset");
    auto length = std::wstring_view(path).size();
    Require(length < output.filename.size(), "Temporary test path is too long");
    for (std::size_t i = 0; i < length; ++i) Require(output.filename[i] == static_cast<char16_t>(path[i]), "File failure record changed its path");
    Require(output.filename[length] == u'\0', "File failure path was not terminated");
    Storage::Handle event(queue->GetErrorEvent());
    Require(WaitForSingleObject(event.Get(), 0) == WAIT_TIMEOUT, "File error retrieval did not reset the event");
}
