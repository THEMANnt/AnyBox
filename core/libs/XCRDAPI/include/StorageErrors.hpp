#ifndef ANYBOX_STORAGE_ERRORS_HPP
#define ANYBOX_STORAGE_ERRORS_HPP

#include <array>
#include <cstddef>
#include <cstdint>

struct IStorageStatusArray;
struct ID3D12Fence;

struct StorageRequestFailure {
    std::uint64_t options;
    void* destination;
    std::uint32_t destinationSize;
    std::array<char16_t, 260> filename;
    std::uint64_t fileOffset;
    void* source;
    std::uint32_t sourceSize;
    std::uint64_t cancellationTag;
    std::array<char, 64> name;
};

struct StorageStatusFailure {
    IStorageStatusArray* array;
    std::uint32_t index;
};

struct StorageSignalFailure {
    ID3D12Fence* fence;
    std::uint64_t value;
};

struct StorageFailure {
    std::int32_t result = 0;
    std::int32_t commandType = -1;
    union {
        StorageRequestFailure request{};
        StorageStatusFailure status;
        StorageSignalFailure signal;
    };
};

struct StorageErrorRecord {
    std::uint32_t failureCount = 0;
    StorageFailure firstFailure;
};

static_assert(sizeof(StorageRequestFailure) == 640);
static_assert(offsetof(StorageRequestFailure, filename) == 20);
static_assert(offsetof(StorageRequestFailure, fileOffset) == 544);
static_assert(offsetof(StorageRequestFailure, source) == 552);
static_assert(offsetof(StorageRequestFailure, cancellationTag) == 568);
static_assert(offsetof(StorageRequestFailure, name) == 576);
static_assert(offsetof(StorageErrorRecord, firstFailure) == 8);
static_assert(sizeof(StorageErrorRecord) == 656);

#endif
