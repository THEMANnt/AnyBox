#include "Queue.hpp"

#include <algorithm>
#include <stdexcept>

void AnyBox::Storage::Queue::recordError(const Operation& operation, std::int32_t result) {
    std::lock_guard lock(mutex);
    if (errorRecord.failureCount == 0) {
        auto& failure = errorRecord.firstFailure;
        failure.result = result;
        if (const auto* read = std::get_if<ReadOperation>(&operation)) {
            failure.commandType = 0;
            const auto& request = read->request;
            auto& output = failure.request;
            output.options = request.options;
            output.destination = request.destination;
            output.destinationSize = request.destinationSize;
            output.fileOffset = request.sourceOffset;
            output.source = description.description.sourceType == 1 ? request.source : nullptr;
            output.sourceSize = request.sourceSize;
            output.cancellationTag = request.cancellationTag;
            output.name = read->name;
            auto path = read->file.GetPath();
            auto length = std::min(path.size(), output.filename.size() - 1);
            for (std::size_t i = 0; i < length; ++i) output.filename[i] = static_cast<char16_t>(path[i]);
        } else if (const auto* signal = std::get_if<SignalOperation>(&operation)) {
            failure.commandType = 2;
            failure.signal = signal->GetFailure();
        } else if (const auto* status = std::get_if<StatusOperation>(&operation)) {
            failure.commandType = 1;
            failure.status = status->GetFailure();
        }
    }
    ++errorRecord.failureCount;
    GetCounters().errors.fetch_add(1, std::memory_order_relaxed);
    if (!SetEvent(errorEvent.Get())) std::terminate();
}

void AnyBox::Storage::Queue::RetrieveErrorRecord(StorageErrorRecord* record) {
    if (!record) throw std::invalid_argument("DirectStorage error record output is null");
    std::lock_guard lock(mutex);
    if (closing) throw std::logic_error("DirectStorage queue is closing");
    if (!ResetEvent(errorEvent.Get())) Windows::ThrowLastError("ResetEvent DirectStorage error");

    *record = errorRecord;
    errorRecord = {};
}
