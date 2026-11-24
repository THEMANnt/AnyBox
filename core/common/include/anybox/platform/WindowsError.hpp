#ifndef ANYBOX_PLATFORM_WINDOWS_ERROR_HPP
#define ANYBOX_PLATFORM_WINDOWS_ERROR_HPP

#include <anybox/platform/Windows.hpp>

#include <system_error>

namespace AnyBox::Windows {

inline void CheckHResult(HRESULT result, const char* operation) {
    if (FAILED(result)) throw std::system_error(static_cast<int>(result), std::system_category(), operation);
}

[[noreturn]] inline void ThrowLastError(const char* operation) {
    auto error = GetLastError();
    throw std::system_error(static_cast<int>(error), std::system_category(), operation);
}

}

#endif
