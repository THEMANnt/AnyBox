#include "ReadLease.hpp"
#include "File.hpp"

#include <utility>

AnyBox::Storage::ReadLease::ReadLease(File* owner, Handle handle) noexcept : owner(owner), handle(std::move(handle)) {}

AnyBox::Storage::ReadLease::ReadLease(ReadLease&& other) noexcept : owner(std::exchange(other.owner, nullptr)), handle(std::move(other.handle)) {}

AnyBox::Storage::ReadLease::~ReadLease() {
    if (!owner) return;
    owner->finishRead();
}

HANDLE AnyBox::Storage::ReadLease::Get() const { return handle.Get(); }

std::wstring_view AnyBox::Storage::ReadLease::GetPath() const { return owner ? std::wstring_view(owner->path) : std::wstring_view{}; }
