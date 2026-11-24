#include "Protection.hpp"
#include "WindowsApi.hpp"
#include "AddressRequirements.hpp"
#include "Validation.hpp"

#include <xmem.hpp>

#include <limits>
#include <stdexcept>

void* XMemVirtualAlloc(void* address, std::size_t size, std::uint32_t allocationType, std::uint64_t attributes, std::uint32_t protection) noexcept(false) {
    if (size == 0) throw std::invalid_argument("XMem virtual allocation size must be nonzero");
    AnyBox::XMem::ValidateVirtualAllocation(allocationType, attributes, protection);
    constexpr std::uint32_t pageModeMask = 0x20400000;
    constexpr std::uint32_t hostMask = MEM_COMMIT | MEM_RESERVE | MEM_REPLACE_PLACEHOLDER | MEM_RESERVE_PLACEHOLDER | MEM_TOP_DOWN | MEM_WRITE_WATCH;
    if ((allocationType & ~(pageModeMask | hostMask)) != 0) throw std::invalid_argument("Unsupported XMem virtual allocation type");
    auto pageMode = allocationType & pageModeMask;
    if (pageMode == 0x400000) throw std::invalid_argument("Invalid XMem page mode");
    std::size_t alignment = pageMode == 0x20000000 ? 0x200000 : 0x10000;
    auto hostType = allocationType & hostMask;
    auto hostProtection = AnyBox::XMem::HostProtection(protection);
    bool reserve = (hostType & MEM_RESERVE) != 0;
    bool replace = (hostType & MEM_REPLACE_PLACEHOLDER) != 0;
    static const auto allocate = AnyBox::XMem::GetMemoryFunction<AnyBox::XMem::AllocateVirtual>("VirtualAlloc2");
    bool placeholder = (hostType & MEM_RESERVE_PLACEHOLDER) != 0 || (pageMode != 0 && reserve && (hostType & MEM_COMMIT) == 0);
    auto pageSize = pageMode == 0 ? std::size_t{4096} : alignment;
    auto requested = reinterpret_cast<std::uintptr_t>(address);
    auto addressAlignment = reserve && pageMode == 0 ? alignment : pageSize;
    auto roundedAddress = requested & ~(addressAlignment - 1);
    auto offset = requested - roundedAddress;
    auto maximum = std::numeric_limits<std::size_t>::max();
    if (size > maximum - offset || size + offset > maximum - (pageSize - 1)) throw std::overflow_error("XMem virtual allocation size overflows");
    size = (size + offset + pageSize - 1) & ~(pageSize - 1);
    if (address && roundedAddress > maximum - size) throw std::overflow_error("XMem virtual allocation address range overflows");
    address = reinterpret_cast<void*>(roundedAddress);
    if (placeholder && replace) {
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(address, &information, sizeof(information))) AnyBox::Windows::ThrowLastError("VirtualQuery placeholder");
        if (information.State != MEM_RESERVE || information.BaseAddress != address || information.RegionSize != size) {
            throw std::invalid_argument("XMem replacement must match an existing placeholder");
        }
        auto* result = allocate(nullptr, address, size, MEM_RESERVE | MEM_REPLACE_PLACEHOLDER, PAGE_NOACCESS, nullptr, 0);
        if (!result) AnyBox::Windows::ThrowLastError("VirtualAlloc2 placeholder validation");
        if (!VirtualFree(result, size, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) AnyBox::Windows::ThrowLastError("VirtualFree preserve placeholder");
        return result;
    }
    if (placeholder) {
        hostType |= MEM_RESERVE_PLACEHOLDER;
        hostProtection = PAGE_NOACCESS;
    }
    auto requirements = AnyBox::XMem::GetAddressRequirements(allocationType, attributes);
    MEM_EXTENDED_PARAMETER parameter{};
    parameter.Type = MemExtendedParameterAddressRequirements;
    parameter.Pointer = &requirements;
    auto useAlignment = !address && (reserve || (hostType & MEM_COMMIT) != 0);
    if (useAlignment && !reserve) hostType |= MEM_RESERVE;
    auto* result = allocate(nullptr, address, size, hostType, hostProtection, useAlignment ? &parameter : nullptr, useAlignment ? 1 : 0);
    if (!result) AnyBox::Windows::ThrowLastError("VirtualAlloc2");
    return result;
}
