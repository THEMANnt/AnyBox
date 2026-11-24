#pragma once

#include <anybox/platform/WindowsError.hpp>
#include <cstring>

namespace AnyBox::Graphics {
class HostLibrary {
public:
    HostLibrary();
    ~HostLibrary();
    HostLibrary(const HostLibrary&) = delete;
    HostLibrary& operator=(const HostLibrary&) = delete;

    template<class TFunction>
    TFunction Get(const char* name) const {
        auto address = GetProcAddress(module, name);
        if (!address) Windows::ThrowLastError(name);
        static_assert(sizeof(TFunction) == sizeof(address));
        TFunction function;
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }

private:
    HMODULE module;
};

const HostLibrary& GetHostLibrary();
void ConfigureHostRuntime(const HostLibrary& library);
}
