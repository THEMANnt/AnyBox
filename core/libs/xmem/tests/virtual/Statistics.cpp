#include "Tests.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

void AnyBox::Tests::CheckStatistics() {
    struct Guarded {
        std::uint64_t before = 0x1A2B3C4D;
        XMemWorkingSetStatistics statistics{};
        std::uint64_t after = 0x4D3C2B1A;
    } output;
    auto& statistics = output.statistics;
    Require(XMemGetWorkingSetStatistics(0x20000001, &statistics) != 0, "XMem statistics failed");
    MEMORYSTATUSEX native{};
    native.dwLength = sizeof(native);
    Require(GlobalMemoryStatusEx(&native) != 0, "Statistics test memory query failed");
    Require(statistics.gameLimit == std::min(native.ullTotalPhys, native.ullTotalVirtual), "XMem memory budget is fabricated");
    Require(statistics.gameUsed > 0 && statistics.gameUsedPeak >= statistics.gameUsed, "XMem private commit statistics are invalid");
    Require(statistics.toolsLimit == 0 && statistics.toolsUsed == 0, "XMem reported unavailable tools memory");
    auto before = statistics.gameUsed;
    {
        auto region = AllocateRegion(0x1000000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        XMemGetWorkingSetStatistics(0x20000001, &statistics);
        Require(statistics.gameUsed >= before + 0x1000000, "XMem did not observe committed host memory");
        Require(statistics.gameUsedPeak >= statistics.gameUsed, "XMem peak commit is below current commit");
    }
    auto during = statistics.gameUsed;
    XMemGetWorkingSetStatistics(0x20000001, &statistics);
    Require(statistics.gameUsed < during, "XMem statistics did not observe released host memory");
    for (auto version : {0U, 0x10000000U, 0x20000000U}) {
        std::array<std::uint64_t, 17> guarded;
        guarded.fill(0xA5A5A5A5A5A5A5A5ULL);
        auto* legacy = reinterpret_cast<XMemWorkingSetStatistics*>(guarded.data() + 1);
        Require(XMemGetWorkingSetStatistics(version | 1, legacy) != 0, "Versioned XMem statistics failed");
        auto words = version == 0x20000000 ? 15 : version == 0x10000000 ? 10 : 9;
        Require(guarded[0] == 0xA5A5A5A5A5A5A5A5ULL, "XMem statistics wrote before its buffer");
        for (std::size_t i = words + 1; i < guarded.size(); ++i) {
            Require(guarded[i] == 0xA5A5A5A5A5A5A5A5ULL, "XMem statistics overran its versioned buffer");
        }
    }
    for (auto flags : {0x20000000U, 0x20000002U, 0x20008002U}) {
        XMemGetWorkingSetStatistics(flags, &statistics);
        Require(statistics.gameLimit == 0 && statistics.gameUsed == 0 && statistics.gameUsedPeak == 0, "Unrequested game statistics were populated");
        Require(statistics.toolsLimit == 0 && statistics.toolsUsed == 0, "Unavailable tools memory was populated");
    }
    XMemGetWorkingSetStatistics(0x20000010, &statistics);
    auto pageTablesBefore = statistics.cpuPageTableSize;
    Require(pageTablesBefore != 0 && pageTablesBefore % 4096 == 0, "XMem page table statistics are not page-sized");
    {
        auto region = AllocateRegion(0x2000000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        XMemGetWorkingSetStatistics(0x20000010, &statistics);
        Require(statistics.cpuPageTableSize >= pageTablesBefore + 15 * 4096, "XMem page table statistics did not observe new committed ranges");
    }
    auto pageTablesDuring = statistics.cpuPageTableSize;
    XMemGetWorkingSetStatistics(0x20000010, &statistics);
    Require(statistics.cpuPageTableSize < pageTablesDuring, "XMem page table statistics did not observe released ranges");
    statistics.gameUsed = 0x571;
    for (auto flags : {0x20000008U, 0x2000000AU}) {
        ExpectException<std::runtime_error>([&] { XMemGetWorkingSetStatistics(flags, &statistics); });
        Require(statistics.gameUsed == 0x571, "Unsupported statistics query changed its output");
    }
    ExpectException<std::invalid_argument>([&] { XMemGetWorkingSetStatistics(0x20000020, &statistics); });
    ExpectException<std::invalid_argument>([] { XMemGetWorkingSetStatistics(0, nullptr); });
    Require(output.before == 0x1A2B3C4D && output.after == 0x4D3C2B1A, "XMem statistics overran its output");
}
