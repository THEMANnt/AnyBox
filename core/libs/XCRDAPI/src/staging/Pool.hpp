#ifndef ANYBOX_STORAGE_STAGING_POOL_HPP
#define ANYBOX_STORAGE_STAGING_POOL_HPP

#include "../metrics/Memory.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace AnyBox::Storage {

class StagingPool;

class StagingLease {
public:
    StagingLease(StagingPool* owner, std::size_t first, std::size_t count, std::span<std::byte> bytes);
    StagingLease(StagingLease&& other) noexcept;
    StagingLease(const StagingLease&) = delete;
    StagingLease& operator=(const StagingLease&) = delete;
    ~StagingLease();
    std::span<std::byte> Get() const;

private:
    StagingPool* owner;
    std::size_t first;
    std::size_t count;
    std::span<std::byte> bytes;
};

class StagingPool {
public:
    StagingPool();
    void SetSize(std::uint32_t size);
    StagingLease Acquire(std::size_t size);
    void RegisterObject();
    void UnregisterObject();

private:
    friend class StagingLease;
    void release(std::size_t first, std::size_t count);
    static constexpr std::size_t chunkSize = 65536;
    std::mutex mutex;
    std::condition_variable available;
    TrackedArray<std::byte> buffer;
    TrackedVector<bool> occupied;
    std::size_t capacity = 0;
    std::size_t objects = 0;
    std::size_t leases = 0;
};

class StorageObjectRegistration {
public:
    StorageObjectRegistration();
    StorageObjectRegistration(const StorageObjectRegistration&) = delete;
    StorageObjectRegistration& operator=(const StorageObjectRegistration&) = delete;
    ~StorageObjectRegistration();
};

StagingPool& GetStagingPool();

}

#endif
