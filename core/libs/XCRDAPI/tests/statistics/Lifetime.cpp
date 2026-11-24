#include "Tests.hpp"

#include <array>
#include <string>

void AnyBox::Tests::CheckStatisticsLifetime() {
    auto factory = GetFactory();
    auto baseline = ReadStatistics(factory.get());
    Require(baseline.currentMemoryUsage >= 32 * 1024 * 1024, "Statistics omitted the default staging buffer");
    std::string name(200, 'q');
    for (std::int8_t priority = -1; priority <= 2; ++priority) {
        auto index = static_cast<std::size_t>(priority + 1);
        StorageQueueDescription1 description{{priority == 2 ? 1ULL : 0ULL, 128, priority, {}, name.c_str()}, 1};
        void* output = nullptr;
        factory->CreateQueue1(&description, Storage::QueueId, &output);
        Object<IStorageQueue> queue(static_cast<IStorageQueue*>(output));
        auto opened = ReadStatistics(factory.get());
        Require(opened.priorities[index].currentQueueCount == baseline.priorities[index].currentQueueCount + 1, "Statistics omitted an open queue");
        Require(opened.priorities[index].cumulativeQueueCount == baseline.priorities[index].cumulativeQueueCount + 1, "Statistics omitted a cumulative queue");
        Require(opened.priorities[index].maximumQueueCount >= opened.priorities[index].currentQueueCount, "Queue peak is below its current count");
        Require(opened.currentMemoryUsage > baseline.currentMemoryUsage + 128 * sizeof(StorageRequest), "Statistics omitted queue storage");
        queue->Close();
        auto closed = ReadStatistics(factory.get());
        Require(closed.priorities[index].currentQueueCount == baseline.priorities[index].currentQueueCount, "Closed queue remained open in statistics");
        Require(closed.currentMemoryUsage > baseline.currentMemoryUsage && closed.currentMemoryUsage <= opened.currentMemoryUsage, "Closing a live queue object corrupted its memory charge");
        queue.reset();
        auto released = ReadStatistics(factory.get());
        Require(released.currentMemoryUsage == baseline.currentMemoryUsage, "Released queue retained its memory charge");
    }
    void* output = nullptr;
    factory->CreateStatusArray(1024, name.c_str(), Storage::StatusArrayId, &output);
    Object<IStorageStatusArray> status(static_cast<IStorageStatusArray*>(output));
    auto live = ReadStatistics(factory.get());
    Require(live.currentMemoryUsage >= baseline.currentMemoryUsage + 1024 * sizeof(std::uint64_t), "Statistics omitted status array storage");
    status.reset();
    auto released = ReadStatistics(factory.get());
    Require(released.currentMemoryUsage == baseline.currentMemoryUsage, "Released status array retained its memory charge");
    Require(released.maximumMemoryUsage >= live.currentMemoryUsage, "Statistics lost its memory high water mark");
    Require(released.currentVirtualDiskCount == 0 && released.cumulativeVirtualDiskCount == 0, "Unpackaged host backend reported Xbox virtual disks");
    ExpectException<std::invalid_argument>([&] { factory->GetStats(nullptr); });

    struct Guarded {
        std::uint64_t before = 0x173A8D473F168B21ULL;
        StorageStatistics statistics{};
        std::uint64_t after = 0x218B163F478D3A17ULL;
    } guarded;
    factory->GetStats(&guarded.statistics);
    Require(guarded.before == 0x173A8D473F168B21ULL && guarded.after == 0x218B163F478D3A17ULL, "Storage statistics overwrote the caller buffer");
}
