#include "Tests.hpp"

#include <array>

void AnyBox::Tests::CheckAddressRanges() {
    struct Range {
        std::uint64_t attributes;
        std::uintptr_t lower;
        std::uintptr_t upper;
    };
    constexpr std::array ranges{
        Range{3, 0x100000000ULL, 0xFFFFFFFFFFULL},
        Range{7, 0x80000000ULL, 0xFFFFFFFFULL},
        Range{0x1001, 0x10000000000ULL, 0x3FFFFFFFFFFFULL},
        Range{0x2001, 0x10000000000ULL, 0x1FFFFFFFFFFFULL},
        Range{0x3001, 0x10000000000ULL, 0xFFFFFFFFFFFULL},
        Range{0x4001, 0x10000000000ULL, 0x7FFFFFFFFFFULL},
        Range{0x5001, 0x10000000000ULL, 0x3FFFFFFFFFFULL},
        Range{0x6001, 0x10000000000ULL, 0x1FFFFFFFFFFULL},
        Range{0x7001, 0x100000000ULL, 0xFFFFFFFFFFULL}
    };
    for (const auto& range : ranges) {
        for (auto type : {MEM_RESERVE | MEM_COMMIT, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, MEM_COMMIT, MEM_COMMIT | MEM_TOP_DOWN}) {
            Region memory(XMemVirtualAlloc(nullptr, 0x10000, type, range.attributes, PAGE_READWRITE));
            auto address = reinterpret_cast<std::uintptr_t>(memory.get());
            Require(address >= range.lower && address <= range.upper - 0xFFFF, "XMem allocation escaped its requested address range");
            auto* bytes = static_cast<std::byte*>(memory.get());
            bytes[0] = std::byte{0x57};
            bytes[0xFFFF] = std::byte{0x39};
            Require(bytes[0] == std::byte{0x57} && bytes[0xFFFF] == std::byte{0x39}, "XMem constrained allocation is not CPU writable");
        }
    }
    Region placeholder(XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, 7, PAGE_NOACCESS));
    auto address = reinterpret_cast<std::uintptr_t>(placeholder.get());
    Require(address >= 0x80000000ULL && address < 0x100000000ULL && address % 0x200000 == 0, "XMem 32-bit placeholder address requirements changed");
    auto* replaced = XMemVirtualAlloc(placeholder.get(), 0x10000, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, 7, PAGE_READWRITE);
    Require(replaced == placeholder.get(), "XMem constrained placeholder replacement moved its address");
    static_cast<std::byte*>(replaced)[0xFFFF] = std::byte{0x31};
}
