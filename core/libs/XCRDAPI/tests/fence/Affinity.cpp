#include "Fence.hpp"
#include "../queue/Tests.hpp"

#include <memory>
#include <system_error>

namespace {

struct ReleaseFence {
    void operator()(AnyBox::Tests::TestFence* fence) const { fence->Release(); }
};

struct RestoreAffinity {
    IStorageFactory* factory;
    DWORD_PTR mask;
    ~RestoreAffinity() { factory->SetCpuAffinity(mask); }
};

class ProcessMaskScope {
public:
    ProcessMaskScope(DWORD_PTR original, DWORD_PTR restricted) : processMask(original) {
        threadMask = SetThreadAffinityMask(GetCurrentThread(), restricted);
        if (!threadMask) AnyBox::Windows::ThrowLastError("SetThreadAffinityMask startup test");
        if (!SetProcessAffinityMask(GetCurrentProcess(), restricted)) {
            auto error = GetLastError();
            if (!SetThreadAffinityMask(GetCurrentThread(), threadMask)) std::terminate();
            throw std::system_error(static_cast<int>(error), std::system_category(), "SetProcessAffinityMask startup test");
        }
    }
    ~ProcessMaskScope() {
        if (!SetProcessAffinityMask(GetCurrentProcess(), processMask)) std::terminate();
        if (!SetThreadAffinityMask(GetCurrentThread(), threadMask)) std::terminate();
    }

private:
    DWORD_PTR processMask;
    DWORD_PTR threadMask;
};

void Verify(AnyBox::Tests::QueueFixture& queue, AnyBox::Tests::TestFence* fence, std::uint64_t value) {
    queue.queue->EnqueueSignal(fence, value);
    queue.Wait();
    AnyBox::Tests::Require(queue.status->GetHResult(0) == 0 && fence->GetCompletedValue() == value, "Worker affinity did not match its requested mask");
}

}

void CheckWorkerAffinity() {
    using namespace AnyBox::Tests;
    auto factory = GetFactory();
    DWORD_PTR processMask = 0;
    DWORD_PTR systemMask = 0;
    Require(GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask) != 0 && processMask != 0, "Worker affinity process query failed");
    RestoreAffinity restore{factory.get(), processMask};
    auto first = processMask & (~processMask + 1);
    if (processMask != first) {
        factory->SetCpuAffinity(processMask);
        StorageStatistics before{};
        factory->GetStats(&before);
        {
            ProcessMaskScope scope(processMask, first);
            ExpectException<std::system_error>([] { QueueFixture invalid(1); });
        }
        StorageStatistics after{};
        factory->GetStats(&after);
        Require(after.currentMemoryUsage == before.currentMemoryUsage, "Failed worker startup leaked tracked memory");
        Require(after.priorities[3].currentQueueCount == before.priorities[3].currentQueueCount, "Failed worker startup registered a queue");
    }
    factory->SetCpuAffinity(first);
    QueueFixture one(1);
    QueueFixture two(1);
    std::unique_ptr<TestFence, ReleaseFence> fence(new TestFence);
    fence->ExpectAffinity(first);
    Verify(one, fence.get(), 1);
    Verify(two, fence.get(), 2);

    factory->SetCpuAffinity(processMask);
    fence->ExpectAffinity(processMask);
    Verify(one, fence.get(), 3);
    Verify(two, fence.get(), 4);
    ExpectException<std::invalid_argument>([&] { factory->SetCpuAffinity(0); });
    Verify(one, fence.get(), 5);

    factory->SetCpuAffinity(first);
    fence->ExpectAffinity(first);
    Verify(two, fence.get(), 6);
    QueueFixture later(1);
    Verify(later, fence.get(), 7);
}
