#include "Tests.hpp"
#include "../fence/Fence.hpp"

#include <bit>

namespace {

struct ReleaseFence {
    void operator()(AnyBox::Tests::TestFence* fence) const { fence->Release(); }
};

}

void AnyBox::Tests::CheckStatisticsFence() {
    QueueFixture fixture(1);
    std::unique_ptr<TestFence, ReleaseFence> fence(new TestFence);
    auto baseline = ReadStatistics(fixture.factory.get());
    fixture.queue->EnqueueSignal(fence.get(), 7);
    auto unpublished = ReadStatistics(fixture.factory.get());
    Require(unpublished.priorities[3].totalFenceCount == baseline.priorities[3].totalFenceCount, "Unpublished fence was counted as submitted");
    fixture.Wait();
    auto completed = ReadStatistics(fixture.factory.get());
    Require(completed.priorities[3].totalFenceCount == baseline.priorities[3].totalFenceCount + 1, "Fence was not counted");
    Require(completed.priorities[3].totalRequestCount == baseline.priorities[3].totalRequestCount, "Fence was counted as a read request");
    fence->SetResult(std::bit_cast<HRESULT>(0x887A0005U));
    fixture.queue->EnqueueSignal(fence.get(), 9);
    fixture.Wait();
    auto failed = ReadStatistics(fixture.factory.get());
    Require(failed.priorities[3].totalFenceCount == baseline.priorities[3].totalFenceCount + 2, "Failed fence was not counted");
    Require(failed.errorCount == baseline.errorCount + 1, "Failed fence was not counted as an error");
    Require(failed.storageErrors == baseline.storageErrors && failed.decompressionErrors == baseline.decompressionErrors, "Fence failure corrupted IO or decompression error counts");
}
