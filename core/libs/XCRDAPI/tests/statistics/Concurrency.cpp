#include "Tests.hpp"

#include <array>
#include <atomic>
#include <exception>
#include <thread>

void AnyBox::Tests::CheckStatisticsConcurrency() {
    auto factory = GetFactory();
    auto baseline = ReadStatistics(factory.get());
    std::array<std::exception_ptr, 4> errors{};
    std::array<std::jthread, 4> threads;
    std::atomic<bool> start{false};
    for (std::size_t index = 0; index < threads.size(); ++index) {
        threads[index] = std::jthread([&, index] {
            start.wait(false);
            try {
                std::array<std::byte, 257> source{};
                std::array<std::array<std::byte, 257>, 64> destinations{};
                source.fill(std::byte(index + 1));
                QueueFixture fixture(1);
                StorageRequest request{};
                request.options = 1ULL << 22;
                request.source = source.data();
                request.sourceSize = static_cast<std::uint32_t>(source.size());
                request.destinationSize = request.sourceSize;
                for (auto& destination : destinations) {
                    request.destination = destination.data();
                    fixture.queue->EnqueueRequest(&request);
                }
                fixture.Wait();
                for (const auto& destination : destinations) Require(destination == source, "Concurrent statistics workload lost a memory request");
            } catch (...) {
                errors[index] = std::current_exception();
            }
        });
    }
    start.store(true);
    start.notify_all();
    for (std::uint32_t repeat = 0; repeat < 100; ++repeat) {
        auto running = ReadStatistics(factory.get());
        Require(running.maximumMemoryUsage >= running.currentMemoryUsage, "Concurrent statistics lost its memory high water mark");
    }
    for (auto& thread : threads) thread.join();
    for (const auto& error : errors) if (error) std::rethrow_exception(error);
    auto completed = ReadStatistics(factory.get());
    Require(completed.currentMemoryUsage == baseline.currentMemoryUsage, "Concurrent queue destruction retained tracked memory");
    const auto& before = baseline.priorities[3];
    const auto& after = completed.priorities[3];
    Require(after.currentQueueCount == before.currentQueueCount && after.pendingRequestCount == 0, "Completed concurrent queues remained open or pending");
    Require(after.cumulativeQueueCount == before.cumulativeQueueCount + 4, "Concurrent queue registrations were lost");
    Require(after.totalRequestCount == before.totalRequestCount + 256, "Concurrent submitted requests were lost");
    Require(after.totalStatusCount == before.totalStatusCount + 4, "Concurrent status writes were lost");
    Require(after.totalSourceBytes == before.totalSourceBytes + 256 * 257 && after.totalDestinationBytes == before.totalDestinationBytes + 256 * 257, "Concurrent request byte counts were lost");
}
