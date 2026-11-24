#ifndef ANYBOX_XMEM_NATIVE_API_HPP
#define ANYBOX_XMEM_NATIVE_API_HPP

#include <anybox/platform/WindowsError.hpp>

#include <cstring>
#include <exception>
#include <memory>
#include <system_error>

namespace AnyBox::XMem {

struct CloseNativeHandle {
    void operator()(HANDLE handle) const noexcept {
        if (!CloseHandle(handle)) std::terminate();
    }
};

using NativeHandle = std::unique_ptr<void, CloseNativeHandle>;

template<class TFunction>
TFunction GetNativeFunction(const char* name) {
    auto module = GetModuleHandleW(L"ntdll.dll");
    if (!module) Windows::ThrowLastError("GetModuleHandleW ntdll");
    auto address = GetProcAddress(module, name);
    if (!address) Windows::ThrowLastError(name);
    static_assert(sizeof(TFunction) == sizeof(address));
    TFunction function;
    std::memcpy(&function, &address, sizeof(function));
    return function;
}

[[noreturn]] inline void ThrowNativeStatus(LONG status, const char* operation) {
    using ConvertStatus = ULONG (WINAPI*)(LONG);
    static const auto convert = GetNativeFunction<ConvertStatus>("RtlNtStatusToDosError");
    throw std::system_error(static_cast<int>(convert(status)), std::system_category(), operation);
}

}

#endif
