#include "StatusArray.hpp"

#include <stdexcept>

AnyBox::Storage::StatusArray::StatusArray(std::uint32_t capacity, const char* name) : capacity(capacity), name(name ? name : "") {
    if (capacity == 0) throw std::invalid_argument("DirectStorage status array capacity is zero");
    entries = MakeTrackedArray<StatusEntry>(capacity);
}

std::int32_t AnyBox::Storage::StatusArray::QueryInterface(const Guid& interfaceId, void** object) {
    if (!object) throw std::invalid_argument("DirectStorage status interface output is null");
    if (interfaceId != StatusArrayId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage status interface");
    AddRef();
    *object = static_cast<IStorageStatusArray*>(this);
    return 0;
}

std::uint32_t AnyBox::Storage::StatusArray::AddRef() {
    return references.Add();
}

std::uint32_t AnyBox::Storage::StatusArray::Release() {
    auto count = references.Release();
    if (count == 0) delete this;
    return count;
}

AnyBox::Storage::StatusEntry& AnyBox::Storage::StatusArray::GetEntry(std::uint32_t index) {
    if (index >= capacity) throw std::out_of_range("DirectStorage status index is outside the array");
    return entries[index];
}

bool AnyBox::Storage::StatusArray::IsComplete(std::uint32_t index) {
    return GetEntry(index).IsComplete();
}

std::int32_t AnyBox::Storage::StatusArray::GetHResult(std::uint32_t index) {
    return GetEntry(index).GetResult();
}
