#ifndef ANYBOX_STORAGE_COUNTERS_HPP
#define ANYBOX_STORAGE_COUNTERS_HPP

#include <StorageStatistics.hpp>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace AnyBox::Storage {

struct PriorityCounters {
    std::atomic<std::uint32_t> currentQueues{0};
    std::atomic<std::uint32_t> maximumQueues{0};
    std::atomic<std::uint32_t> cumulativeQueues{0};
    std::atomic<std::uint32_t> statusCount{0};
    std::atomic<std::uint32_t> fenceCount{0};
    std::atomic<std::uint32_t> pendingRequests{0};
    std::atomic<std::uint32_t> totalRequests{0};
    std::atomic<std::uint64_t> sourceBytes{0};
    std::atomic<std::uint64_t> destinationBytes{0};
};

struct Counters {
    std::array<PriorityCounters, 4> priorities;
    std::atomic<std::uint32_t> pendingStorage{0};
    std::atomic<std::uint32_t> totalStorage{0};
    std::atomic<std::uint32_t> storageErrors{0};
    std::atomic<std::uint32_t> pendingDecompression{0};
    std::atomic<std::uint32_t> totalDecompression{0};
    std::atomic<std::uint32_t> decompressionErrors{0};
    std::atomic<std::uint32_t> stagingStalls{0};
    std::atomic<std::uint32_t> currentFiles{0};
    std::atomic<std::uint32_t> maximumFiles{0};
    std::atomic<std::uint32_t> cumulativeFiles{0};
    std::atomic<std::uint64_t> currentMemory{0};
    std::atomic<std::uint64_t> maximumMemory{0};
    std::atomic<std::uint32_t> errors{0};
};

Counters& GetCounters();
StorageStatistics GetStatistics();

template<class TValue>
void UpdateMaximum(std::atomic<TValue>& maximum, TValue value) noexcept {
    auto current = maximum.load(std::memory_order_relaxed);
    while (current < value && !maximum.compare_exchange_weak(current, value, std::memory_order_relaxed)) {}
}

}

#endif
