#ifndef ANYBOX_XMEM_WINDOWS_API_HPP
#define ANYBOX_XMEM_WINDOWS_API_HPP

#include <anybox/platform/WindowsError.hpp>

#include <cstring>

namespace AnyBox::XMem {

template<class TFunction>
TFunction GetMemoryFunction(const char* name) {
    auto module = GetModuleHandleW(L"kernelbase.dll");
    if (!module) Windows::ThrowLastError("GetModuleHandleW kernelbase");
    auto address = GetProcAddress(module, name);
    if (!address) Windows::ThrowLastError(name);
    static_assert(sizeof(TFunction) == sizeof(address));
    TFunction function;
    std::memcpy(&function, &address, sizeof(function));
    return function;
}

using AllocateVirtual = void* (WINAPI*)(HANDLE, void*, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
using MapSection = void* (WINAPI*)(HANDLE, HANDLE, void*, ULONG64, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG);
using UnmapSection = BOOL (WINAPI*)(HANDLE, void*, ULONG);

}

#endif
