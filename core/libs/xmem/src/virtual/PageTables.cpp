#include "Statistics.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

std::uint32_t AnyBox::XMem::GetPageTableSize() {
    constexpr std::uintptr_t coverage = 0x200000;
    SYSTEM_INFO system{};
    GetSystemInfo(&system);
    auto maximum = reinterpret_cast<std::uintptr_t>(system.lpMaximumApplicationAddress);
    std::uintptr_t address = 0;
    std::uintptr_t coveredEnd = 0;
    std::uint64_t coveredBytes = 0;

    while (address <= maximum) {
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(reinterpret_cast<const void*>(address), &information, sizeof(information))) Windows::ThrowLastError("VirtualQuery XMem page tables");
        if (information.RegionSize == 0 || information.RegionSize > std::numeric_limits<std::uintptr_t>::max() - address) throw std::overflow_error("XMem page table address range overflows");
        auto next = address + information.RegionSize;
        if (information.State == MEM_COMMIT) {
            auto begin = std::max(coveredEnd, address);
            if (next > std::numeric_limits<std::uintptr_t>::max() - coverage) throw std::overflow_error("XMem page table coverage overflows");
            coveredEnd = (next + coverage) & ~(coverage - 1);
            if (begin < next) coveredBytes += coveredEnd - (begin & ~(coverage - 1));
        }
        address = next;
    }
    auto bytes = coveredBytes >> 9;
    if (bytes > std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("XMem page table size exceeds the ABI range");
    return static_cast<std::uint32_t>(bytes);
}
