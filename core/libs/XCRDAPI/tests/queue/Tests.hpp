#ifndef ANYBOX_STORAGE_QUEUE_TESTS_HPP
#define ANYBOX_STORAGE_QUEUE_TESTS_HPP

#include "../file/Tests.hpp"
#include "../../src/common/Handle.hpp"

namespace AnyBox::Tests {

class QueueFixture {
public:
    explicit QueueFixture(std::uint64_t sourceType) : factory(GetFactory()), event(CreateEventW(nullptr, TRUE, FALSE, nullptr)) {
        if (!event.Get()) Windows::ThrowLastError("CreateEventW queue test");
        StorageQueueDescription1 description{{sourceType, 128, static_cast<std::int8_t>(sourceType == 1 ? 2 : 0), {}, "Queue test"}, 1};
        void* output = nullptr;
        Require(factory->CreateQueue1(&description, Storage::QueueId, &output) == 0, "Queue creation failed");
        queue.reset(static_cast<IStorageQueue*>(output));
        Require(factory->CreateStatusArray(1, "Queue test", Storage::StatusArrayId, &output) == 0, "Queue status creation failed");
        status.reset(static_cast<IStorageStatusArray*>(output));
    }
    void Wait() {
        if (!ResetEvent(event.Get())) Windows::ThrowLastError("ResetEvent queue test");
        queue->EnqueueStatus(status.get(), 0);
        queue->EnqueueSetEvent(event.Get());
        queue->Submit();
        Require(WaitForSingleObject(event.Get(), 5000) == WAIT_OBJECT_0, "Queue did not complete");
        Require(status->IsComplete(0), "Queue event arrived before its status");
    }
    Object<IStorageFactory> factory;
    Object<IStorageQueue> queue;
    Object<IStorageStatusArray> status;
    Storage::Handle event;
};

void CheckMemoryQueue();
void CheckFileQueue();
void CheckQueueLimits();
void CheckCancellation();
void CheckQueueInformation();
void CheckQueueErrors();
void CheckFileErrorRecord(IStorageQueue* queue, const StorageRequest& request, const wchar_t* path);

}

#endif
