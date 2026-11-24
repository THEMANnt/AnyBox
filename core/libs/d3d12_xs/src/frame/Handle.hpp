#pragma once

#include <anybox/platform/WindowsError.hpp>
#include <memory>
#include <stdexcept>

namespace AnyBox::Graphics {
struct HandleCloser {
    void operator()(void* value) const noexcept { CloseHandle(value); }
};
using FrameHandle = std::unique_ptr<void, HandleCloser>;

inline FrameHandle OwnFrameHandle(HANDLE value, const char* operation) {
    if (!value || value == INVALID_HANDLE_VALUE) Windows::ThrowLastError(operation);
    return FrameHandle(value);
}

inline void SignalFrameEvent(HANDLE event) {
    if (!SetEvent(event)) Windows::ThrowLastError("SetEvent Xbox frame event");
}

inline bool SignalFrameSemaphore(HANDLE semaphore) {
    if (ReleaseSemaphore(semaphore, 1, nullptr)) return true;
    if (GetLastError() == ERROR_TOO_MANY_POSTS) return false;
    Windows::ThrowLastError("ReleaseSemaphore Xbox frame event");
}
}
