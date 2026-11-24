#ifndef ANYBOX_STORAGE_STATISTICS_HPP
#define ANYBOX_STORAGE_STATISTICS_HPP

#include <array>
#include <cstddef>
#include <cstdint>

struct StoragePriorityStatistics {
    std::uint32_t currentQueueCount;
    std::uint32_t maximumQueueCount;
    std::uint32_t cumulativeQueueCount;
    std::uint32_t totalStatusCount;
    std::uint32_t totalFenceCount;
    std::uint32_t pendingRequestCount;
    std::uint32_t totalRequestCount;
    std::uint64_t totalSourceBytes;
    std::uint64_t totalDestinationBytes;
};

struct StorageStatistics {
    std::array<StoragePriorityStatistics, 4> priorities;
    std::uint8_t currentVirtualDiskCount;
    std::uint8_t maximumVirtualDiskCount;
    std::uint32_t cumulativeVirtualDiskCount;
    std::uint32_t pendingStorageRequestCount;
    std::uint32_t totalStorageRequestCount;
    std::uint32_t storageRetries;
    std::uint32_t storageErrors;
    std::uint32_t pendingDecompressionRequestCount;
    std::uint32_t totalDecompressionRequestCount;
    std::uint32_t decompressionErrors;
    std::uint32_t stagingBufferStallCount;
    std::uint32_t currentFileCount;
    std::uint32_t maximumFileCount;
    std::uint32_t cumulativeFileCount;
    std::uint32_t currentMemoryUsage;
    std::uint32_t maximumMemoryUsage;
    std::uint32_t errorCount;
};

static_assert(sizeof(StoragePriorityStatistics) == 48);
static_assert(offsetof(StoragePriorityStatistics, totalSourceBytes) == 32);
static_assert(offsetof(StorageStatistics, currentVirtualDiskCount) == 192);
static_assert(offsetof(StorageStatistics, pendingStorageRequestCount) == 200);
static_assert(offsetof(StorageStatistics, currentFileCount) == 232);
static_assert(offsetof(StorageStatistics, currentMemoryUsage) == 244);
static_assert(offsetof(StorageStatistics, errorCount) == 252);
static_assert(sizeof(StorageStatistics) == 256);

#endif
