#ifndef ANYBOX_XMEM_TYPES_HPP
#define ANYBOX_XMEM_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

struct XMemWorkingSetStatistics {
    std::uint64_t gameLimit;
    std::uint64_t gameUsed;
    std::uint64_t toolsLimit;
    std::uint64_t toolsUsed;
    std::uint32_t stackUsed;
    std::uint32_t gameAllocatedPhysicalPageCount;
    std::uint64_t gpuOptimalBandwidthLimit;
    std::uint64_t gpuOptimalBandwidthUsed;
    std::uint64_t toolsGpuOptimalBandwidthLimit;
    std::uint64_t toolsGpuOptimalBandwidthUsed;
    std::uint32_t kernelPoolUsed;
    std::uint32_t kernelPoolLimit;
    std::uint32_t directStorageUsed;
    std::uint32_t framePreserveBufferSize;
    std::uint64_t gameUsedPeak;
    std::uint32_t gpuPageTableSize;
    std::uint32_t cpuPageTableSize;
    std::array<std::uint64_t, 2> reserved;
};

struct XMemAllocationHook;
struct XMemFreeHook;

struct XMemGraphicsMemoryConfiguration {
    std::uint32_t pageTableSize;
    std::uint32_t framePreserveBufferSize;
    std::array<std::uintptr_t, 2> backendData;
};

struct XMemMemoryBasicInformation {
    void* baseAddress;
    void* allocationBase;
    std::uint32_t allocationProtection;
    std::uint16_t partitionId;
    std::uint16_t reserved0;
    std::size_t regionSize;
    std::uint32_t state;
    std::uint32_t protection;
    std::uint32_t type;
    std::uint32_t reserved1;
    std::uint32_t accessFlags;
    std::array<std::uint32_t, 3> reserved2;
};

static_assert(sizeof(XMemWorkingSetStatistics) == 120);
static_assert(sizeof(XMemGraphicsMemoryConfiguration) == 24);
static_assert(offsetof(XMemGraphicsMemoryConfiguration, backendData) == 8);
static_assert(offsetof(XMemWorkingSetStatistics, gameUsed) == 8);
static_assert(sizeof(XMemMemoryBasicInformation) == 64);
static_assert(offsetof(XMemMemoryBasicInformation, regionSize) == 24);
static_assert(offsetof(XMemMemoryBasicInformation, accessFlags) == 48);

#endif
