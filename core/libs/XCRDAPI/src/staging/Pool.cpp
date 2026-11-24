#include "Pool.hpp"
#include "../metrics/Counters.hpp"

#include <algorithm>
#include <stdexcept>

AnyBox::Storage::StagingPool::StagingPool() {
    SetSize(32 * 1024 * 1024);
}

void AnyBox::Storage::StagingPool::SetSize(std::uint32_t size) {
    constexpr std::uint32_t unit = 1024 * 1024;
    if (size != 0 && (size < 16 * unit || size > 32 * unit || size % (4 * unit) != 0)) throw std::invalid_argument("Invalid DirectStorage staging buffer size");
    std::lock_guard lock(mutex);
    if (objects != 0 || leases != 0) throw std::logic_error("DirectStorage staging buffer cannot change while objects exist");
    auto replacement = size ? MakeTrackedArray<std::byte>(size) : TrackedArray<std::byte>{};
    TrackedVector<bool> slots((size + chunkSize - 1) / chunkSize, false);
    buffer = std::move(replacement);
    occupied.swap(slots);
    capacity = size;
}

AnyBox::Storage::StagingLease AnyBox::Storage::StagingPool::Acquire(std::size_t size) {
    std::unique_lock lock(mutex);
    if (size == 0 || size > capacity) throw std::length_error("DirectStorage request exceeds staging buffer capacity");
    auto count = (size + chunkSize - 1) / chunkSize;
    auto first = occupied.size();
    bool stalled = false;
    available.wait(lock, [&] {
        std::size_t free = 0;
        for (std::size_t i = 0; i < occupied.size(); ++i) {
            free = occupied[i] ? 0 : free + 1;
            if (free == count) {
                first = i + 1 - count;
                return true;
            }
        }
        if (!stalled) GetCounters().stagingStalls.fetch_add(1, std::memory_order_relaxed);
        stalled = true;
        return false;
    });
    std::fill_n(occupied.begin() + first, count, true);
    ++leases;
    return StagingLease(this, first, count, {buffer.get() + first * chunkSize, size});
}

void AnyBox::Storage::StagingPool::release(std::size_t first, std::size_t count) {
    std::lock_guard lock(mutex);
    if (leases == 0 || first > occupied.size() || count > occupied.size() - first) std::terminate();
    std::fill_n(occupied.begin() + first, count, false);
    --leases;
    available.notify_all();
}

AnyBox::Storage::StagingPool& AnyBox::Storage::GetStagingPool() {
    static StagingPool pool;
    return pool;
}

void AnyBox::Storage::StagingPool::RegisterObject() {
    std::lock_guard lock(mutex);
    ++objects;
}

void AnyBox::Storage::StagingPool::UnregisterObject() {
    std::lock_guard lock(mutex);
    if (objects == 0) std::terminate();
    --objects;
}
