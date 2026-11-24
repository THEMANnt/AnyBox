#include "Tests.hpp"

#include <array>
#include <cstddef>

void AnyBox::Tests::CheckCancellation() {
    std::array<std::byte, 257> source;
    std::array<std::byte, 257> canceled;
    std::array<std::byte, 257> retained{};
    std::array<std::byte, 257> later{};
    source.fill(std::byte{0x71});
    canceled.fill(std::byte{0x39});
    QueueFixture test(1);
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destinationSize = request.sourceSize;
    request.destination = canceled.data();
    request.cancellationTag = 0xA5;
    test.queue->EnqueueRequest(&request);
    request.destination = retained.data();
    request.cancellationTag = 0xB5;
    test.queue->EnqueueRequest(&request);
    test.queue->CancelRequestsWithTag(0xF0, 0xA0);

    request.destination = later.data();
    request.cancellationTag = 0xA5;
    test.queue->EnqueueRequest(&request);
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "Canceled DirectStorage request incorrectly failed its status group");
    for (auto byte : canceled) Require(byte == std::byte{0x39}, "Canceled unpublished request wrote destination data");
    Require(retained == source, "Cancellation removed a request with a different tag");
    Require(later == source, "Cancellation affected a request enqueued after cancellation");
    Storage::Handle error(test.queue->GetErrorEvent());
    Require(WaitForSingleObject(error.Get(), 0) == WAIT_TIMEOUT, "Cancellation incorrectly signaled the error event");
}
