#ifndef ANYBOX_STORAGE_FACTORY_HPP
#define ANYBOX_STORAGE_FACTORY_HPP

#include "../common/ReferenceCount.hpp"
#include "../staging/Pool.hpp"

#include <StorageInterfaces.hpp>

namespace AnyBox::Storage {

class Factory final : public IStorageFactory {
public:
    std::int32_t QueryInterface(const Guid& interfaceId, void** object) override;
    std::uint32_t AddRef() override;
    std::uint32_t Release() override;
    std::int32_t CreateQueue(const StorageQueueDescription* description, const Guid& interfaceId, void** queue) override;
    std::int32_t OpenFile(const wchar_t* path, const Guid& interfaceId, void** file) override;
    std::int32_t CreateStatusArray(std::uint32_t capacity, const char* name, const Guid& interfaceId, void** statusArray) override;
    std::int32_t SetCpuAffinity(std::uint64_t affinity) override;
    void SetDebugFlags(std::uint32_t flags) override;
    std::int32_t SetStagingBufferSize(std::uint32_t size) override;
    void GetStats(StorageStatistics* statistics) override;
    std::int32_t CreateQueue1(const StorageQueueDescription1* description, const Guid& interfaceId, void** queue) override;

private:
    ReferenceCount references{0};
    StagingPool& stagingPool = GetStagingPool();
};

Factory& GetFactory();

}

#endif
