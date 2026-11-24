#include "Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <system_error>

void AnyBox::Tests::CheckQuery() {
    auto region = AllocateRegion(0x10000, MEM_RESERVE, PAGE_READWRITE);
    struct Guarded {
        std::uint64_t before = 0x1122334455667788;
        XMemMemoryBasicInformation information{};
        std::uint64_t after = 0x8877665544332211;
    } output;
    auto process = GetCurrentProcess();
    auto& information = output.information;
    Require(XMemVirtualQuery(process, region.get(), &information) != 0, "XMem reserve query failed");
    Require(information.state == MEM_RESERVE && information.accessFlags == 0, "Reserved XMem memory is accessible");
    Require(information.allocationBase == region.get() && information.regionSize >= 0x10000, "XMem reservation geometry changed");
    auto* page = static_cast<std::byte*>(region.get()) + 4096;
    Require(VirtualAlloc(page, 4096, MEM_COMMIT, PAGE_READWRITE) == page, "Query test page commit failed");
    XMemVirtualQuery(process, page, &information);
    Require(information.baseAddress == page && information.regionSize == 4096, "XMem committed subregion geometry changed");
    Require(information.state == MEM_COMMIT && information.protection == PAGE_READWRITE && information.accessFlags == 3, "XMem writable access flags changed");
    DWORD previous = 0;
    Require(VirtualProtect(page, 4096, PAGE_READONLY, &previous) != 0, "Read-only test protection failed");
    XMemVirtualQuery(process, page, &information);
    Require(information.accessFlags == 1 && information.protection == PAGE_READONLY, "XMem read-only access flags changed");
    Require(VirtualProtect(page, 4096, PAGE_READWRITE | PAGE_GUARD, &previous) != 0, "Guard test protection failed");
    XMemVirtualQuery(process, page, &information);
    Require(information.accessFlags == 0 && (information.protection & PAGE_GUARD) != 0, "XMem guard page was reported accessible");
    Require(VirtualProtect(page, 4096, PAGE_NOACCESS, &previous) != 0, "No-access test protection failed");
    XMemVirtualQuery(process, page, &information);
    Require(information.accessFlags == 0 && information.protection == PAGE_NOACCESS, "XMem no-access page was reported accessible");
    auto saved = information;
    ExpectException<std::system_error>([&] { XMemVirtualQuery(reinterpret_cast<void*>(std::uintptr_t{0xDEAD}), page, &information); });
    Require(std::memcmp(&saved, &information, sizeof(saved)) == 0, "Failed XMem query modified its output");
    ExpectException<std::invalid_argument>([&] { XMemVirtualQuery(nullptr, page, &information); });
    ExpectException<std::invalid_argument>([&] { XMemVirtualQuery(process, page, nullptr); });
    Require(output.before == 0x1122334455667788 && output.after == 0x8877665544332211, "XMem query overran its 64-byte output");
}
