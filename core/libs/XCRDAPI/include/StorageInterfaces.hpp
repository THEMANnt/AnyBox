#ifndef ANYBOX_STORAGE_INTERFACES_HPP
#define ANYBOX_STORAGE_INTERFACES_HPP

#include <anybox/Types.hpp>
#include <StorageTypes.hpp>
#include <StorageStatistics.hpp>

#include <cstdint>

struct StorageQueueDescription;
struct StorageQueueDescription1;
struct ID3D12Fence;

struct IStorageUnknown {
    virtual std::int32_t QueryInterface(const AnyBox::Guid& interfaceId, void** object) = 0;
    virtual std::uint32_t AddRef() = 0;
    virtual std::uint32_t Release() = 0;
};

struct IStorageFile : IStorageUnknown {
    virtual void Close() = 0;
    virtual void* GetHandle() = 0;
};

struct IStorageStatusArray : IStorageUnknown {
    virtual bool IsComplete(std::uint32_t index) = 0;
    virtual std::int32_t GetHResult(std::uint32_t index) = 0;
};

struct IStorageQueue : IStorageUnknown {
    virtual void EnqueueRequest(const StorageRequest* request) = 0;
    virtual void EnqueueStatus(IStorageStatusArray* array, std::uint32_t index) = 0;
    virtual void EnqueueSignal(ID3D12Fence* fence, std::uint64_t value) = 0;
    virtual void Submit() = 0;
    virtual void CancelRequestsWithTag(std::uint64_t mask, std::uint64_t value) = 0;
    virtual void Close() = 0;
    virtual void* GetErrorEvent() = 0;
    virtual void RetrieveErrorRecord(StorageErrorRecord* record) = 0;
    virtual void Query(StorageQueueInformation* information) = 0;
    virtual void EnqueueSetEvent(void* event) = 0;
    virtual void Query1(StorageQueueInformation1* information) = 0;
};

struct IStorageFactory : IStorageUnknown {
    virtual std::int32_t CreateQueue(const StorageQueueDescription* description, const AnyBox::Guid& interfaceId, void** queue) = 0;
    virtual std::int32_t OpenFile(const wchar_t* path, const AnyBox::Guid& interfaceId, void** file) = 0;
    virtual std::int32_t CreateStatusArray(std::uint32_t capacity, const char* name, const AnyBox::Guid& interfaceId, void** statusArray) = 0;
    virtual std::int32_t SetCpuAffinity(std::uint64_t affinity) = 0;
    virtual void SetDebugFlags(std::uint32_t flags) = 0;
    virtual std::int32_t SetStagingBufferSize(std::uint32_t size) = 0;
    virtual void GetStats(StorageStatistics* statistics) = 0;
    virtual std::int32_t CreateQueue1(const StorageQueueDescription1* description, const AnyBox::Guid& interfaceId, void** queue) = 0;
};

namespace AnyBox::Storage {

inline constexpr Guid UnknownId{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
inline constexpr Guid FactoryId{0x62959D11, 0xD45F, 0x4884, {0xA0, 0xA3, 0xF7, 0x86, 0x2E, 0xA2, 0x43, 0xAE}};
inline constexpr Guid FileId{0x43704F9F, 0x27C6, 0x4A9F, {0x88, 0x15, 0xD1, 0x65, 0x27, 0x89, 0x4D, 0xF4}};
inline constexpr Guid StatusArrayId{0xCD66D752, 0x2786, 0x44F7, {0x90, 0x70, 0xE2, 0xA6, 0xFD, 0xAD, 0x3F, 0x1B}};
inline constexpr Guid QueueId{0x3C02CB0C, 0xC050, 0x4195, {0x99, 0xA1, 0xA1, 0xCE, 0x94, 0xBA, 0x33, 0xC5}};

}

#endif
