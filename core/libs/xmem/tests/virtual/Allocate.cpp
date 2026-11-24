#include "Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>

void AnyBox::Tests::CheckVirtualAllocation() {
    auto process = GetCurrentProcess();
    XMemMemoryBasicInformation information{};
    for (auto mode : {0U, 0x20000000U, 0x20400000U}) {
        Region region(XMemVirtualAlloc(nullptr, 0x10000, mode | MEM_RESERVE | MEM_COMMIT, mode == 0 ? 3 : 0x183, 0x22004));
        auto alignment = mode == 0x20000000 ? 0x200000U : 0x10000U;
        Require(reinterpret_cast<std::uintptr_t>(region.get()) % alignment == 0, "XMem page mode alignment changed");
        XMemVirtualQuery(process, region.get(), &information);
        Require(information.state == MEM_COMMIT && information.protection == PAGE_READWRITE, "XMem graphics protection corrupted CPU access");
        std::memset(region.get(), 0x49, 0x10000);
    }
    Region reservation(XMemVirtualAlloc(nullptr, 0x20000, MEM_RESERVE, 7, PAGE_NOACCESS));
    XMemVirtualQuery(process, reservation.get(), &information);
    Require(information.state == MEM_RESERVE && information.accessFlags == 0, "XMem reserved memory was committed");
    auto* committed = XMemVirtualAlloc(reservation.get(), 4096, MEM_COMMIT, 7, PAGE_READWRITE);
    Require(committed == reservation.get(), "XMem commit moved its address");
    std::memset(committed, 0x37, 4096);
    Require(VirtualFree(committed, 4096, MEM_DECOMMIT) != 0, "XMem memory was incompatible with VirtualFree decommit");
    XMemVirtualQuery(process, committed, &information);
    Require(information.state == MEM_RESERVE, "XMem region did not decommit");
    for (auto type : {0x42000U, 0x20402000U}) {
        Region placeholder(XMemVirtualAlloc(nullptr, 0x20000, type, type == 0x42000 ? 3 : 0x23, 0x2004));
        XMemVirtualQuery(process, placeholder.get(), &information);
        Require(information.state == MEM_RESERVE && information.accessFlags == 0, "XMem placeholder is not reserved");
        Require(VirtualFree(placeholder.get(), 0x10000, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER) != 0, "XMem placeholder could not be split");
        Region second(static_cast<std::byte*>(placeholder.get()) + 0x10000);
        Require(XMemVirtualAlloc(placeholder.get(), 0x10000, 0x20406000, 0x27, 0x2004) == placeholder.get(), "XMem physical window replacement moved its address");
        auto* replaced = XMemVirtualAlloc(placeholder.get(), 0x10000, MEM_RESERVE | MEM_COMMIT | MEM_REPLACE_PLACEHOLDER, 7, PAGE_READWRITE);
        Require(replaced == placeholder.get(), "XMem placeholder replacement moved its address");
        std::memset(replaced, 0x15, 0x10000);
    }
    Region unalignedReservation(XMemVirtualAlloc(nullptr, 0x10000, MEM_RESERVE, 7, PAGE_NOACCESS));
    auto* unaligned = static_cast<std::byte*>(unalignedReservation.get()) + 4093;
    Require(XMemVirtualAlloc(unaligned, 8, MEM_COMMIT, 7, PAGE_READWRITE) == unalignedReservation.get(), "XMem commit did not round its address down");
    std::memset(unaligned, 0x53, 8);
    XMemVirtualQuery(process, unaligned + 7, &information);
    Require(information.state == MEM_COMMIT, "XMem rounded range omitted the final page");
    auto* overflowing = reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max() - 4096);
    ExpectException<std::overflow_error>([&] { XMemVirtualAlloc(overflowing, 8192, MEM_COMMIT, 1, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 0, MEM_RESERVE, 0, PAGE_NOACCESS); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 4096, 0x80000000, 0, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 4096, MEM_RESERVE, 0x80000000, PAGE_READWRITE); });
    ExpectException<std::invalid_argument>([] { XMemVirtualAlloc(nullptr, 4096, MEM_RESERVE, 0, 0); });
    ExpectException<std::overflow_error>([] { XMemVirtualAlloc(nullptr, std::numeric_limits<std::size_t>::max(), 0x20402000, 1, PAGE_NOACCESS); });
}
