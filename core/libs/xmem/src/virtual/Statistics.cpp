#include "Statistics.hpp"
#include "../graphics/Configuration.hpp"

#include <xmem.hpp>
#include <anybox/Unimplemented.hpp>
#include <anybox/platform/WindowsError.hpp>

#include <psapi.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

std::int32_t XMemGetWorkingSetStatistics(std::uint32_t flags, XMemWorkingSetStatistics* statistics) noexcept(false) {
    if (!statistics) throw std::invalid_argument("XMem working set output is null");
    if ((flags & 0x0FFF7FE0U) != 0) throw std::invalid_argument("Invalid XMem working set statistics flags");
    if ((flags & 8) != 0) ANYBOX_NOT_IMPLEMENTED();
    XMemWorkingSetStatistics result{};
    if ((flags & 1) != 0) {
        PROCESS_MEMORY_COUNTERS_EX counters{};
        counters.cb = sizeof(counters);
        if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
            AnyBox::Windows::ThrowLastError("GetProcessMemoryInfo");
        }
        MEMORYSTATUSEX memory{};
        memory.dwLength = sizeof(memory);
        if (!GlobalMemoryStatusEx(&memory)) AnyBox::Windows::ThrowLastError("GlobalMemoryStatusEx");
        result.gameLimit = std::min(memory.ullTotalPhys, memory.ullTotalVirtual);
        result.gameUsed = counters.PrivateUsage;
        result.gameUsedPeak = counters.PeakPagefileUsage;
    }

    if ((flags & 4) != 0) result.stackUsed = AnyBox::XMem::GetStackSize();
    if ((flags & 0x10) != 0) result.cpuPageTableSize = AnyBox::XMem::GetPageTableSize();
    AnyBox::XMem::GetGraphicsStatistics(result);

    auto version = flags & 0xF0000000;
    std::size_t outputSize = version == 0x20000000 ? 120 : version == 0x10000000 ? 80 : 72;
    if (outputSize == 72) result.gameAllocatedPhysicalPageCount = result.kernelPoolUsed;
    std::memcpy(statistics, &result, outputSize);
    return 1;
}
