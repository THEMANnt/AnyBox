#ifndef ANYBOX_XMEM_VIRTUAL_TESTS_HPP
#define ANYBOX_XMEM_VIRTUAL_TESTS_HPP

#include "../heap/Tests.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <exception>
#include <memory>

namespace AnyBox::Tests {

struct ReleaseRegion {
    void operator()(void* address) const noexcept {
        if (!VirtualFree(address, 0, MEM_RELEASE)) std::terminate();
    }
};

using Region = std::unique_ptr<void, ReleaseRegion>;

inline Region AllocateRegion(std::size_t size, DWORD allocationType, DWORD protection) {
    auto* address = VirtualAlloc(nullptr, size, allocationType, protection);
    if (!address) AnyBox::Windows::ThrowLastError("VirtualAlloc test allocation");
    return Region(address);
}

void CheckQuery();
void CheckVirtualAllocation();
void CheckAddressRanges();
void CheckVirtualValidation();
void CheckStatistics();
void CheckStackStatistics();
void CheckTransfer();

}

#endif
