#include "Queue.hpp"

#include <stdexcept>

AnyBox::Storage::Queue::Queue(const StorageQueueDescription1& input) : description(input), name(input.description.name ? input.description.name : "") {
    if (description.description.sourceType > 1 || description.description.capacity < 128) throw std::invalid_argument("Invalid DirectStorage queue source or capacity");
    if (description.description.priority < -1 || description.description.priority > 2) throw std::invalid_argument("Invalid DirectStorage queue priority");
    if (description.description.sourceType == 1 && description.description.priority != 2) throw std::invalid_argument("Memory-sourced DirectStorage queues require real-time priority");
    if ((description.options & ~1ULL) != 0) throw std::invalid_argument("Unsupported DirectStorage queue options");
    description.description.name = name.c_str();
    operations.resize(description.description.capacity);
    errorEvent = Handle(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!errorEvent.Get()) Windows::ThrowLastError("CreateEventW DirectStorage error");
    readEvent = Handle(CreateEventW(nullptr, TRUE, FALSE, nullptr));
    if (!readEvent.Get()) Windows::ThrowLastError("CreateEventW DirectStorage read");
    worker = std::jthread([this] { run(); });
    std::unique_lock lock(mutex);
    wake.wait(lock, [this] { return started; });
    if (startupError) std::rethrow_exception(startupError);
    statisticsRegistration.Open(description.description.priority);
}

AnyBox::Storage::Queue::~Queue() {
    Close();
}

std::int32_t AnyBox::Storage::Queue::QueryInterface(const Guid& interfaceId, void** object) {
    if (!object) throw std::invalid_argument("DirectStorage queue output is null");
    if (interfaceId != QueueId && interfaceId != UnknownId) throw std::invalid_argument("Unsupported DirectStorage queue interface");
    AddRef();
    *object = static_cast<IStorageQueue*>(this);
    return 0;
}

std::uint32_t AnyBox::Storage::Queue::AddRef() { return references.Add(); }

std::uint32_t AnyBox::Storage::Queue::Release() {
    auto remaining = references.Release();
    if (remaining == 0) delete this;
    return remaining;
}

void AnyBox::Storage::Queue::Submit() {
    std::lock_guard lock(mutex);
    if (closing) throw std::logic_error("DirectStorage queue is closing");
    submitted = count;
    wake.notify_one();
}

void AnyBox::Storage::Queue::Close() {
    std::unique_lock lock(mutex);
    if (closed) return;
    closing = true;
    spaceAvailable.notify_all();
    submitted = count;
    wake.notify_one();
    drained.wait(lock, [this] { return closed; });
}

void* AnyBox::Storage::Queue::GetErrorEvent() {
    return errorEvent.Duplicate().Release();
}
