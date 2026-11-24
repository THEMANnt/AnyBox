#ifndef ANYBOX_STORAGE_STATUS_ARRAY_HPP
#define ANYBOX_STORAGE_STATUS_ARRAY_HPP

#include "Entry.hpp"
#include "../common/ReferenceCount.hpp"
#include "../metrics/Memory.hpp"

#include <StorageInterfaces.hpp>

#include <memory>
#include <string>

namespace AnyBox::Storage {

class StatusArray final : public IStorageStatusArray, public TrackedObject {
public:
    StatusArray(std::uint32_t capacity, const char* name);
    std::int32_t QueryInterface(const Guid& interfaceId, void** object) override;
    std::uint32_t AddRef() override;
    std::uint32_t Release() override;
    bool IsComplete(std::uint32_t index) override;
    std::int32_t GetHResult(std::uint32_t index) override;
    StatusEntry& GetEntry(std::uint32_t index);

private:
    ReferenceCount references{1};
    std::uint32_t capacity;
    TrackedArray<StatusEntry> entries;
    TrackedString<char> name;
};

}

#endif
