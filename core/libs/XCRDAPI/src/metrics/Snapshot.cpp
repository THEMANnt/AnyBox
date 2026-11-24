#include "Counters.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

StorageStatistics AnyBox::Storage::GetStatistics() {
    auto& counters = GetCounters();
    StorageStatistics result{};
    for (std::size_t index = 0; index < result.priorities.size(); ++index) {
        auto& input = counters.priorities[index];
        auto& output = result.priorities[index];
        output.currentQueueCount = input.currentQueues.load(std::memory_order_relaxed);
        output.maximumQueueCount = std::max(output.currentQueueCount, input.maximumQueues.load(std::memory_order_relaxed));
        output.cumulativeQueueCount = input.cumulativeQueues.load(std::memory_order_relaxed);
        output.totalStatusCount = input.statusCount.load(std::memory_order_relaxed);
        output.totalFenceCount = input.fenceCount.load(std::memory_order_relaxed);
        output.pendingRequestCount = input.pendingRequests.load(std::memory_order_relaxed);
        output.totalRequestCount = input.totalRequests.load(std::memory_order_relaxed);
        output.totalSourceBytes = input.sourceBytes.load(std::memory_order_relaxed);
        output.totalDestinationBytes = input.destinationBytes.load(std::memory_order_relaxed);
    }
    result.pendingStorageRequestCount = counters.pendingStorage.load(std::memory_order_relaxed);
    result.totalStorageRequestCount = counters.totalStorage.load(std::memory_order_relaxed);
    result.storageErrors = counters.storageErrors.load(std::memory_order_relaxed);
    result.pendingDecompressionRequestCount = counters.pendingDecompression.load(std::memory_order_relaxed);
    result.totalDecompressionRequestCount = counters.totalDecompression.load(std::memory_order_relaxed);
    result.decompressionErrors = counters.decompressionErrors.load(std::memory_order_relaxed);
    result.stagingBufferStallCount = counters.stagingStalls.load(std::memory_order_relaxed);
    result.currentFileCount = counters.currentFiles.load(std::memory_order_relaxed);
    result.maximumFileCount = std::max(result.currentFileCount, counters.maximumFiles.load(std::memory_order_relaxed));
    result.cumulativeFileCount = counters.cumulativeFiles.load(std::memory_order_relaxed);
    auto memory = counters.currentMemory.load(std::memory_order_relaxed);
    auto peak = std::max(memory, counters.maximumMemory.load(std::memory_order_relaxed));
    if (peak > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("DirectStorage memory usage exceeds the statistics ABI range");
    result.currentMemoryUsage = static_cast<std::uint32_t>(memory);
    result.maximumMemoryUsage = static_cast<std::uint32_t>(peak);
    result.errorCount = counters.errors.load(std::memory_order_relaxed);
    return result;
}
