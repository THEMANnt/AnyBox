#include "Fence.hpp"
#include "../queue/Tests.hpp"

#include <bit>
#include <iostream>

void CheckWorkerAffinity();

namespace {

struct ReleaseFence {
    void operator()(AnyBox::Tests::TestFence* fence) const { fence->Release(); }
};

void CheckFence() {
    using namespace AnyBox::Tests;
    QueueFixture test(1);
    std::unique_ptr<TestFence, ReleaseFence> fence(new TestFence);
    std::byte source{0x73};
    std::byte destination{};
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = &source;
    request.destination = &destination;
    request.sourceSize = 1;
    request.destinationSize = 1;
    fence->ExpectRead(&destination, source);
    test.queue->EnqueueRequest(&request);
    test.queue->EnqueueSignal(fence.get(), 0xA713900000000005ULL);
    Require(fence->GetReferenceCount() == 2, "Queue did not retain its pending fence");
    Require(fence->GetCompletedValue() == 0 && fence->GetSignalCount() == 0, "Queue signaled an unpublished fence");
    test.Wait();
    Require(test.status->GetHResult(0) == 0, "Ordered fence signal failed");
    Require(fence->GetCompletedValue() == 0xA713900000000005ULL, "Queue changed the 64-bit fence value");
    Require(fence->GetReferenceCount() == 1 && fence->GetSignalCount() == 1, "Queue did not release its completed fence");

    constexpr auto failure = std::bit_cast<HRESULT>(0x887A0005U);
    fence->SetResult(failure);
    test.queue->EnqueueSignal(fence.get(), 9);
    test.Wait();
    Require(test.status->GetHResult(0) == failure, "Queue changed the fence failure HRESULT");
    StorageErrorRecord record;
    test.queue->RetrieveErrorRecord(&record);
    Require(record.failureCount == 1 && record.firstFailure.commandType == 2, "Fence failure record is missing");
    Require(record.firstFailure.result == failure, "Fence failure record lost its HRESULT");
    Require(record.firstFailure.signal.fence == fence.get() && record.firstFailure.signal.value == 9, "Fence failure record changed its parameters");
    Require(fence->GetReferenceCount() == 1, "Failed signal leaked its fence reference");

    fence->SetInterfaceSupported(false);
    ExpectException<std::invalid_argument>([&] { test.queue->EnqueueSignal(fence.get(), 20); });
    Require(fence->GetReferenceCount() == 1, "Rejected fence interface leaked a reference");
    fence->SetInterfaceSupported(true);
    fence->SetResult(S_OK);
    test.queue->EnqueueSignal(fence.get(), 3);
    test.Wait();
    Require(test.status->GetHResult(0) == 0 && fence->GetCompletedValue() == 3, "Queue could not signal a lower fence value after an error");
    ExpectException<std::invalid_argument>([&] { test.queue->EnqueueSignal(nullptr, 1); });
    test.queue->EnqueueSignal(fence.get(), 17);
    test.queue->Close();
    Require(fence->GetCompletedValue() == 17 && fence->GetReferenceCount() == 1, "Queue Close did not drain and release its fence");
    ExpectException<std::logic_error>([&] { test.queue->EnqueueSignal(fence.get(), 20); });
    Require(fence->GetReferenceCount() == 1, "Rejected signal leaked its fence reference");
}

}

int main() {
    try {
        CheckFence();
        CheckWorkerAffinity();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
