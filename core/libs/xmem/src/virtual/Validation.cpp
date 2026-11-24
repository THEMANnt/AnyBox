#include "Validation.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <stdexcept>

void AnyBox::XMem::ValidateVirtualAllocation(std::uint32_t allocationType, std::uint64_t attributes, std::uint32_t protection) {
    if ((attributes & ~0x73B7ULL) != 0) throw std::invalid_argument("Unsupported XMem virtual allocation attributes");
    if ((allocationType & 0x1880000) != 0) throw std::invalid_argument("XMem virtual allocations do not support reset, reset undo or rotation");
    if ((attributes & 1) == 0) throw std::invalid_argument("XMem virtual allocation requires a CPU or graphics memory type");
    if ((attributes & 2) != 0 && (protection & PAGE_NOCACHE) != 0) throw std::invalid_argument("XMem graphics allocations do not support uncached CPU protection");
    auto pageMode = allocationType & 0x20400000;
    if ((attributes & 0x20) != 0) {
        if ((allocationType & (MEM_RESERVE | MEM_COMMIT)) != MEM_RESERVE) throw std::invalid_argument("XMem mappable memory requires an uncommitted reservation");
        if ((allocationType & MEM_RESERVE_PLACEHOLDER) != 0) throw std::invalid_argument("XMem mappable memory cannot be combined with placeholder reservation");
        if (pageMode != 0x20400000 && pageMode != 0x20000000) throw std::invalid_argument("XMem mappable memory requires 64 KiB or 2 MiB pages");
    }
    if ((attributes & 0x10) != 0) {
        if ((pageMode & 0x20000000) == 0) throw std::invalid_argument("XMem tools memory requires 64 KiB or 2 MiB pages");
        throw std::system_error(ERROR_NOT_FOUND, std::system_category(), "XMem tools memory is unavailable");
    }
    if ((attributes & 0x80) != 0 && pageMode == 0) throw std::invalid_argument("XMem bandwidth flags require 64 KiB or 2 MiB pages");
}
