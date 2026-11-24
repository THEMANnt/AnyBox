#ifndef ANYBOX_STORAGE_TYPES_HPP
#define ANYBOX_STORAGE_TYPES_HPP

#include "StorageErrors.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

struct StorageQueueDescription {
    std::uint64_t sourceType;
    std::uint16_t capacity;
    std::int8_t priority;
    std::array<std::uint8_t, 5> padding;
    const char* name;
};

struct StorageQueueDescription1 {
    StorageQueueDescription description;
    std::uint64_t options;
};

struct StorageRequest {
    std::uint64_t options;
    void* destination;
    std::uint16_t destinationPageOffset;
    std::array<std::uint8_t, 6> padding0;
    std::uint32_t destinationSize;
    std::uint32_t padding1;
    void* source;
    std::uint64_t sourceOffset;
    std::uint32_t sourceSize;
    std::uint32_t intermediateSize;
    std::uint64_t cancellationTag;
    const char* name;
};

struct StorageQueueInformation {
    StorageQueueDescription description;
    std::uint16_t emptySlotCount;
    std::uint16_t requestCountUntilAutoSubmit;
};

struct StorageQueueInformation1 {
    StorageQueueDescription1 description;
    std::uint16_t emptySlotCount;
    std::uint16_t requestCountUntilAutoSubmit;
};

static_assert(sizeof(StorageQueueDescription) == 24);
static_assert(sizeof(StorageQueueDescription1) == 32);
static_assert(offsetof(StorageQueueDescription, capacity) == 8);
static_assert(offsetof(StorageQueueDescription, name) == 16);
static_assert(sizeof(StorageQueueInformation) == 32);
static_assert(sizeof(StorageQueueInformation1) == 40);
static_assert(offsetof(StorageQueueInformation, emptySlotCount) == 24);
static_assert(offsetof(StorageQueueInformation1, emptySlotCount) == 32);
static_assert(sizeof(StorageRequest) == 72);
static_assert(offsetof(StorageRequest, source) == 32);
static_assert(offsetof(StorageRequest, sourceSize) == 48);

#endif
