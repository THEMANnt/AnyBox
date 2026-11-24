#ifndef ANYBOX_STORAGE_QUEUE_HPP
#define ANYBOX_STORAGE_QUEUE_HPP

#include "Operation.hpp"
#include "../common/ReferenceCount.hpp"
#include "../metrics/Memory.hpp"
#include "../metrics/Lifetime.hpp"
#include "../compression/ZlibDecoder.hpp"
#include "../compression/ZlibStream.hpp"

#include <StorageInterfaces.hpp>
#include "../staging/Pool.hpp"

#include <condition_variable>
#include <exception>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace AnyBox::Storage {

class Queue final : public IStorageQueue, public TrackedObject {
public:
    explicit Queue(const StorageQueueDescription1& description);
    ~Queue();
    std::int32_t QueryInterface(const Guid& interfaceId, void** object) override;
    std::uint32_t AddRef() override;
    std::uint32_t Release() override;
    void EnqueueRequest(const StorageRequest* request) override;
    void EnqueueStatus(IStorageStatusArray* array, std::uint32_t index) override;
    void EnqueueSignal(ID3D12Fence* fence, std::uint64_t value) override;
    void Submit() override;
    void CancelRequestsWithTag(std::uint64_t mask, std::uint64_t value) override;
    void Close() override;
    void* GetErrorEvent() override;
    void RetrieveErrorRecord(StorageErrorRecord* record) override;
    void Query(StorageQueueInformation* information) override;
    void EnqueueSetEvent(void* event) override;
    void Query1(StorageQueueInformation1* information) override;

private:
    void requireSpace(std::unique_lock<std::mutex>& lock);
    void publish();
    void run();
    void process();
    void recordError(const Operation& operation, std::int32_t result);
    void read(ReadOperation& operation);
    void readMemory(ReadOperation& operation);
    void startStream(MemoryView& destination);
    void processStream(std::span<const std::byte> source, bool finalChunk);
    std::span<std::byte> captureSource(const MemoryView& source);
    void readFile(ReadOperation& operation, std::span<std::byte> destination, std::uint64_t offset = 0);
    void decompressFile(ReadOperation& operation, MemoryView& destination);
    void decompress(std::span<const std::byte> source, std::span<std::byte> destination);
    StorageObjectRegistration registration;
    QueueStatistics statisticsRegistration;
    ReferenceCount references{1};
    StorageQueueDescription1 description;
    TrackedString<char> name;
    std::mutex mutex;
    std::condition_variable wake;
    std::condition_variable drained;
    std::condition_variable spaceAvailable;
    TrackedVector<Operation> operations;
    std::size_t head = 0;
    std::size_t count = 0;
    std::size_t submitted = 0;
    bool started = false;
    std::exception_ptr startupError;
    bool closing = false;
    bool closed = false;
    std::optional<ZlibDecoder> decoder;
    std::optional<ZlibStream> streamDecoder;
    TrackedArray<std::byte> overlapBuffer;
    std::size_t overlapCapacity = 0;
    StorageErrorRecord errorRecord;
    Handle errorEvent;
    Handle readEvent;
    std::jthread worker;
};

}

#endif
