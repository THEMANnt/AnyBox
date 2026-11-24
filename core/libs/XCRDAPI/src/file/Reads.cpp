#include "File.hpp"

#include <stdexcept>

AnyBox::Storage::ReadLease AnyBox::Storage::File::AcquireRead() {
    std::lock_guard lock(mutex);
    if (closing || handle == INVALID_HANDLE_VALUE) throw std::logic_error("DirectStorage file is closed");
    HANDLE duplicate;
    auto process = GetCurrentProcess();
    if (!DuplicateHandle(process, handle, process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) Windows::ThrowLastError("DuplicateHandle DirectStorage read");
    Handle owned(duplicate);
    ++pendingReads;
    return ReadLease(this, std::move(owned));
}

void AnyBox::Storage::File::finishRead() {
    std::lock_guard lock(mutex);
    if (pendingReads == 0) std::terminate();
    --pendingReads;
    if (pendingReads == 0) readsFinished.notify_all();
}
