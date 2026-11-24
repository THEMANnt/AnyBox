#ifndef ANYBOX_STORAGE_HANDLE_HPP
#define ANYBOX_STORAGE_HANDLE_HPP

#include <anybox/platform/WindowsError.hpp>

#include <exception>
#include <utility>

namespace AnyBox::Storage {

class Handle {
public:
    Handle() = default;
    explicit Handle(HANDLE value) : value(value) {}
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept : value(std::exchange(other.value, nullptr)) {}
    Handle& operator=(Handle&& other) noexcept {
        Handle replacement(std::move(other));
        std::swap(value, replacement.value);
        return *this;
    }
    ~Handle() {
        if (value && value != INVALID_HANDLE_VALUE && !CloseHandle(value)) std::terminate();
    }
    HANDLE Get() const { return value; }
    Handle Duplicate() const {
        HANDLE duplicate;
        auto process = GetCurrentProcess();
        if (!DuplicateHandle(process, value, process, &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS)) Windows::ThrowLastError("DuplicateHandle DirectStorage");
        return Handle(duplicate);
    }
    HANDLE Release() { return std::exchange(value, nullptr); }

private:
    HANDLE value = nullptr;
};

}

#endif
