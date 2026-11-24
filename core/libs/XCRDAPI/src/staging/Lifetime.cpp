#include "Pool.hpp"

#include <utility>

AnyBox::Storage::StagingLease::StagingLease(StagingPool* owner, std::size_t first, std::size_t count, std::span<std::byte> bytes) : owner(owner), first(first), count(count), bytes(bytes) {}

AnyBox::Storage::StagingLease::StagingLease(StagingLease&& other) noexcept : owner(std::exchange(other.owner, nullptr)), first(other.first), count(other.count), bytes(other.bytes) {}

AnyBox::Storage::StagingLease::~StagingLease() {
    if (owner) owner->release(first, count);
}

std::span<std::byte> AnyBox::Storage::StagingLease::Get() const { return bytes; }

AnyBox::Storage::StorageObjectRegistration::StorageObjectRegistration() {
    GetStagingPool().RegisterObject();
}

AnyBox::Storage::StorageObjectRegistration::~StorageObjectRegistration() {
    GetStagingPool().UnregisterObject();
}
