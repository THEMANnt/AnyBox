#include "Queue.hpp"
#include "../metrics/Requests.hpp"
#include "../file/File.hpp"

#include <limits>
#include <stdexcept>

namespace {

AnyBox::Storage::ReadLease RetainFile(const StorageRequest& request, std::uint64_t sourceType) {
    if (sourceType == 1) return {};
    if (!request.source) throw std::invalid_argument("DirectStorage file source is null");
    auto* file = dynamic_cast<AnyBox::Storage::File*>(static_cast<IStorageFile*>(request.source));
    if (!file) throw std::invalid_argument("DirectStorage file does not belong to this runtime");
    return file->AcquireRead();
}

}

void AnyBox::Storage::Queue::requireSpace(std::unique_lock<std::mutex>& lock) {
    spaceAvailable.wait(lock, [this] { return closing || count < operations.size() - 1; });
    if (closing) throw std::logic_error("DirectStorage queue is closing");
}

void AnyBox::Storage::Queue::publish() {
    ++count;
    if ((description.options & 1) == 0 && count - submitted >= operations.size() / 2) submitted = count;
    if (submitted != 0) wake.notify_one();
}

void AnyBox::Storage::Queue::EnqueueRequest(const StorageRequest* request) {
    RequestValidation validation;
    if (!request) throw std::invalid_argument("DirectStorage request is null");
    if ((request->options & ~((1ULL << 21) | (1ULL << 22) | (1ULL << 23) | 1)) != 0) throw std::invalid_argument("DirectStorage request transformation is not implemented");
    if (((request->options >> 22) & 1) != description.description.sourceType) throw std::invalid_argument("DirectStorage request source does not match its queue");
    bool compressed = (request->options & 1) != 0;
    if (compressed && description.description.sourceType == 0 && request->sourceOffset % 16 != 0) throw std::invalid_argument("DirectStorage compressed file offset must be 16-byte aligned");
    if (!compressed && request->sourceSize != request->destinationSize) throw std::invalid_argument("DirectStorage uncompressed request sizes differ");
    if (compressed && (request->sourceSize == 0 || request->destinationSize < request->sourceSize)) throw std::invalid_argument("Invalid DirectStorage compressed request sizes");
    if (request->sourceSize != 0 && (!request->source || !request->destination)) throw std::invalid_argument("DirectStorage request buffer is null");
    bool sourcePhysical = (request->options & (1ULL << 23)) != 0;
    bool destinationPhysical = (request->options & (1ULL << 21)) != 0;
    if (sourcePhysical && description.description.sourceType != 1) throw std::invalid_argument("Physical DirectStorage sources require a memory queue");
    ReadOperation operation{*request, RetainFile(*request, description.description.sourceType), {}, {}};
    operation.destination = MemoryView(request->destination, request->destinationSize, destinationPhysical, request->destinationPageOffset);
    if (description.description.sourceType == 1) operation.source = MemoryView(request->source, request->sourceSize, sourcePhysical, static_cast<std::uint16_t>(request->sourceOffset));
    if (request->name) {
        for (std::size_t i = 0; i + 1 < operation.name.size() && request->name[i] != '\0'; ++i) operation.name[i] = request->name[i];
    }
    std::unique_lock lock(mutex);
    requireSpace(lock);
    operations[(head + count) % operations.size()].emplace<ReadOperation>(std::move(operation));
    publish();
}

void AnyBox::Storage::Queue::EnqueueStatus(IStorageStatusArray* array, std::uint32_t index) {
    RequestValidation validation;
    auto* status = dynamic_cast<StatusArray*>(array);
    if (!status) throw std::invalid_argument("DirectStorage status array does not belong to this runtime");
    std::unique_lock lock(mutex);
    requireSpace(lock);
    operations[(head + count) % operations.size()].emplace<StatusOperation>(status, index);
    publish();
}

void AnyBox::Storage::Queue::EnqueueSetEvent(void* event) {
    RequestValidation validation;
    Handle borrowed(event);
    Handle owned;
    try {
        owned = borrowed.Duplicate();
    } catch (...) {
        borrowed.Release();
        throw;
    }
    borrowed.Release();
    std::unique_lock lock(mutex);
    requireSpace(lock);
    operations[(head + count) % operations.size()].emplace<EventOperation>(EventOperation{std::move(owned)});
    publish();
}

void AnyBox::Storage::Queue::CancelRequestsWithTag(std::uint64_t mask, std::uint64_t value) {
    std::lock_guard lock(mutex);
    if (closing) throw std::logic_error("DirectStorage queue is closing");
    for (std::size_t i = 0; i < count; ++i) {
        auto* read = std::get_if<ReadOperation>(&operations[(head + i) % operations.size()]);
        if (read && (read->request.cancellationTag & mask) == value) read->canceled = true;
    }
}

void AnyBox::Storage::Queue::EnqueueSignal(ID3D12Fence* fence, std::uint64_t value) {
    RequestValidation validation;
    SignalOperation operation(fence, value);
    std::unique_lock lock(mutex);
    requireSpace(lock);
    operations[(head + count) % operations.size()].emplace<SignalOperation>(std::move(operation));
    publish();
}
