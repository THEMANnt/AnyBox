#include "Library.hpp"

AnyBox::Graphics::HostLibrary::HostLibrary() : module(LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)) {
    if (!module) Windows::ThrowLastError("LoadLibraryExW d3d12.dll");
    try { ConfigureHostRuntime(*this); } catch (...) { FreeLibrary(module); throw; }
}

AnyBox::Graphics::HostLibrary::~HostLibrary() { FreeLibrary(module); }

const AnyBox::Graphics::HostLibrary& AnyBox::Graphics::GetHostLibrary() {
    static HostLibrary library;
    return library;
}
