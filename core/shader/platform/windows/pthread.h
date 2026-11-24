#pragma once

#include <anybox/platform/Windows.hpp>
#include <cerrno>
#include <stdexcept>
#include <system_error>

using pthread_key_t = DWORD;

inline int pthread_key_delete(pthread_key_t key) {
    if (!FlsFree(key)) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Shader thread storage deletion failed");
    return 0;
}

inline int pthread_key_create(pthread_key_t* key, void (*destroy)(void*)) {
    if (!key) throw std::invalid_argument("Shader thread storage key output is null");
    auto allocated = FlsAlloc(destroy);
    if (allocated == FLS_OUT_OF_INDEXES) return EAGAIN;
    *key = allocated;
    return 0;
}

inline void* pthread_getspecific(pthread_key_t key) {
    SetLastError(ERROR_SUCCESS);
    auto* value = FlsGetValue(key);
    auto error = GetLastError();
    if (!value && error != ERROR_SUCCESS) throw std::system_error(static_cast<int>(error), std::system_category(), "Shader thread storage lookup failed");
    return value;
}

inline int pthread_setspecific(pthread_key_t key, const void* value) {
    if (!FlsSetValue(key, const_cast<void*>(value))) throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "Shader thread storage registration failed");
    return 0;
}
