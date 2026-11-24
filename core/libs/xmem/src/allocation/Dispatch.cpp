#include "TitleHooks.hpp"

#include <xmem.hpp>

void* XMemAlloc(std::size_t size, std::uint64_t attributes) noexcept(false) {
    const auto& hooks = AnyBox::XMem::GetTitleHooks();
    if (hooks.Allocate) return hooks.Allocate(size, attributes);
    return XMemAllocDefault(size, attributes);
}

void XMemFree(void* allocation, std::uint64_t attributes) noexcept(false) {
    const auto& hooks = AnyBox::XMem::GetTitleHooks();
    if (hooks.Free) {
        hooks.Free(allocation, attributes);
        return;
    }
    XMemFreeDefault(allocation, attributes);
}
