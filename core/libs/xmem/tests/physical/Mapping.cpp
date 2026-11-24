#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>

int main() {
    try {
        using namespace AnyBox::Tests;
        CheckPhysicalPageModes();
        std::size_t count = 2;
        std::array<std::uint64_t, 2> frames{};
        Require(XMemAllocatePhysicalPages(0x280, &count, frames.data()) != 0 && count == 2, "Physical allocation count changed");
        Require(frames[0] != frames[1] && frames[0] % 16 == 0 && frames[1] % 16 == 0, "Physical frame identities are invalid");
        auto* range = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, 0x20000, 0x20402000, 0x23, 0x2004));
        Require(XMemMapPhysicalPages(range, 2, frames.data()) == range, "Contiguous physical mapping failed");
        auto* alias = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, 0x10000, 0x20402000, 0x23, 0x2004));
        XMemMapPhysicalPages(alias, 1, frames.data());
        for (std::size_t i = 0; i < 0x20000; ++i) Require(range[i] == std::byte{0}, "New physical pages contain stale data");
        std::memset(range, 0x31, 0x10000);
        for (std::size_t i = 0; i < 0x10000; ++i) Require(alias[i] == std::byte{0x31}, "Physical aliases do not share storage");
        alias[123] = std::byte{0x73};
        Require(range[123] == std::byte{0x73}, "Physical alias writes are not coherent");
        std::memset(range + 0x10000, 0x42, 0x10000);
        std::array<void*, 2> reversedAddresses{range + 0x10000, range};
        XMemMapPhysicalPagesScatter(reversedAddresses.data(), 2, frames.data());
        Require(range[0] == std::byte{0x42} && range[0x10000] == std::byte{0x31}, "Scatter mapping did not reorder physical pages");
        Require(alias[123] == std::byte{0x73}, "Remapping corrupted another alias");
        ExpectException<std::logic_error>([&] { XMemFreePhysicalPages(2, frames.data()); });
        std::array<void*, 2> duplicated{range, range};
        ExpectException<std::invalid_argument>([&] { XMemMapPhysicalPagesScatter(duplicated.data(), 2, frames.data()); });
        auto foreign = AllocateRegion(0x10000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        std::array<void*, 2> invalidDestination{range, foreign.get()};
        ExpectException<std::invalid_argument>([&] { XMemMapPhysicalPagesScatter(invalidDestination.data(), 2, frames.data()); });
        Require(range[0] == std::byte{0x42}, "Failed mapping did not roll back a replaced page");
        XMemMapPhysicalPages(range, 2, nullptr);
        XMemMapPhysicalPages(alias, 1, nullptr);
        XMemMemoryBasicInformation information{};
        XMemVirtualQuery(GetCurrentProcess(), range, &information);
        Require(information.state == MEM_RESERVE && information.accessFlags == 0, "Physical unmap did not restore a placeholder");
        XMemMapPhysicalPages(range, 2, frames.data());
        Require(range[0] == std::byte{0x31} && range[0x10000] == std::byte{0x42}, "Unmapping discarded physical page contents");
        XMemMapPhysicalPages(range, 2, nullptr);
        Require(XMemFreePhysicalPages(2, frames.data()) != 0, "Physical free did not return success");
        ExpectException<std::invalid_argument>([&] { XMemMapPhysicalPages(alias, 1, frames.data()); });
        ExpectException<std::invalid_argument>([&] { XMemFreePhysicalPages(2, frames.data()); });
        Require(VirtualFree(range, 0, MEM_RELEASE) != 0, "Physical first placeholder release failed");
        Require(VirtualFree(range + 0x10000, 0, MEM_RELEASE) != 0, "Physical second placeholder release failed");
        Require(VirtualFree(alias, 0, MEM_RELEASE) != 0, "Physical alias placeholder release failed");
        auto sentinel = frames;
        ExpectException<std::invalid_argument>([&] { XMemAllocatePhysicalPages(1ULL << 40, &count, frames.data()); });
        ExpectException<std::invalid_argument>([] { XMemAllocatePhysicalPages(0, nullptr, nullptr); });
        Require(frames == sentinel && count == 2, "Rejected physical allocation changed its outputs");
        Require(XMemFreePhysicalPages(0, nullptr) != 0, "Empty physical free did not return success");
        Require(XMemMapPhysicalPagesScatter(reversedAddresses.data(), 0, nullptr) != 0, "Empty scatter operation failed");
        ExpectException<std::invalid_argument>([] { XMemMapPhysicalPagesScatter(nullptr, 0, nullptr); });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
