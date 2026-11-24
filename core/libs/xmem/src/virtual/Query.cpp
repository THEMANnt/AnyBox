#include <xmem.hpp>
#include <anybox/platform/WindowsError.hpp>

#include <stdexcept>

namespace {

std::uint32_t AccessFlags(const MEMORY_BASIC_INFORMATION& information) {
    if (information.State != MEM_COMMIT || (information.Protect & PAGE_GUARD) != 0) return 0;
    auto protection = information.Protect & 0xFF;
    switch (protection) {
    case PAGE_READONLY:
    case PAGE_EXECUTE_READ:
        return 1;
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return 3;
    default:
        return 0;
    }
}

}

std::int32_t XMemVirtualQuery(void* process, const void* address, XMemMemoryBasicInformation* information) noexcept(false) {
    if (!process || !information) throw std::invalid_argument("XMemVirtualQuery requires a process and output");
    MEMORY_BASIC_INFORMATION native{};
    if (!VirtualQueryEx(process, address, &native, sizeof(native))) AnyBox::Windows::ThrowLastError("VirtualQueryEx");
    XMemMemoryBasicInformation result{};
    result.baseAddress = native.BaseAddress;
    result.allocationBase = native.AllocationBase;
    result.allocationProtection = native.AllocationProtect;
    result.regionSize = native.RegionSize;
    result.state = native.State;
    result.protection = native.Protect;
    result.type = native.Type;
    result.accessFlags = AccessFlags(native);
    *information = result;
    return 1;
}
