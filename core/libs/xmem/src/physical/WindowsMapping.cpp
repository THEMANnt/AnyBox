#include "Pool.hpp"
#include "../virtual/WindowsApi.hpp"

#include <stdexcept>

namespace {

void Map(void* address, const AnyBox::XMem::Page& page) {
    static const auto map = AnyBox::XMem::GetMemoryFunction<AnyBox::XMem::MapSection>("MapViewOfFile3");
    auto* result = map(page.section->handle, nullptr, address, page.offset, AnyBox::XMem::PhysicalPageSize, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, nullptr, 0);
    if (!result) AnyBox::Windows::ThrowLastError("MapViewOfFile3 physical page");
}

void Unmap(void* address) {
    static const auto unmap = AnyBox::XMem::GetMemoryFunction<AnyBox::XMem::UnmapSection>("UnmapViewOfFile2");
    if (!unmap(GetCurrentProcess(), address, MEM_PRESERVE_PLACEHOLDER)) AnyBox::Windows::ThrowLastError("UnmapViewOfFile2 physical page");
}

}

void AnyBox::XMem::EnsurePlaceholder(void* address) {
    MEMORY_BASIC_INFORMATION information{};
    if (!VirtualQuery(address, &information, sizeof(information))) Windows::ThrowLastError("VirtualQuery physical window");
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto base = reinterpret_cast<std::uintptr_t>(information.BaseAddress);
    if (information.State != MEM_RESERVE || value < base || value - base > information.RegionSize || PhysicalPageSize > information.RegionSize - (value - base)) {
        throw std::invalid_argument("XMem physical mapping requires a reserved window");
    }
    if (base != value) {
        if (!VirtualFree(information.BaseAddress, value - base, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) Windows::ThrowLastError("VirtualFree split physical window prefix");
    }
    if (information.RegionSize - (value - base) > PhysicalPageSize) {
        if (!VirtualFree(address, PhysicalPageSize, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) Windows::ThrowLastError("VirtualFree split physical window suffix");
    }
}

void AnyBox::XMem::ApplyMapping(const MappingChange& change) {
    if (change.previous) Unmap(change.address);
    else EnsurePlaceholder(change.address);
    try {
        if (change.next) Map(change.address, *change.next);
    } catch (...) {
        if (change.previous) Map(change.address, *change.previous);
        throw;
    }
}

void AnyBox::XMem::RestoreMapping(const MappingChange& change) {
    if (change.next) Unmap(change.address);
    if (change.previous) Map(change.address, *change.previous);
}
