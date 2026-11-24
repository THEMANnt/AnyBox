#include "../virtual/Tests.hpp"

#include <anybox/PhysicalMemory.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>

namespace {

struct ReleaseLease {
    void operator()(AnyBox::IPhysicalPageLease* lease) const noexcept { lease->Release(); }
};

using Lease = std::unique_ptr<AnyBox::IPhysicalPageLease, ReleaseLease>;

void CheckLeases() {
    using namespace AnyBox::Tests;
    std::size_t count = 3;
    std::array<std::uint64_t, 3> frames{};
    XMemAllocatePhysicalPages(0x280, &count, frames.data());
    {
        Lease contiguous(AnyBoxAcquirePhysicalPages(frames.size(), frames.data()));
        Require(contiguous->GetPageCount() == 3, "Physical lease changed its page count");
        auto range = contiguous->GetContiguousMemory();
        Require(range.size() == 3 * AnyBox::PhysicalPageSize, "Contiguous physical pages did not expose a single range");
        std::fill(range.begin(), range.end(), std::byte{0x17});
        contiguous->GetPage(1)[3] = std::byte{0x71};
        Require(range[AnyBox::PhysicalPageSize + 3] == std::byte{0x71}, "Physical lease pages do not share their contiguous range");
        ExpectException<std::out_of_range>([&] { contiguous->GetPage(3); });
        ExpectException<std::logic_error>([&] { XMemFreePhysicalPages(frames.size(), frames.data()); });

        std::array<std::uint64_t, 3> reordered{frames[1], frames[0], frames[1]};
        Lease scatter(AnyBoxAcquirePhysicalPages(reordered.size(), reordered.data()));
        reordered.fill(0);
        Require(scatter->GetContiguousMemory().empty(), "Reordered physical pages incorrectly exposed a contiguous range");
        Require(scatter->GetPage(0)[3] == std::byte{0x71}, "Physical lease did not retain its input page array");
        scatter->GetPage(2)[4] = std::byte{0x35};
        Require(scatter->GetPage(0)[4] == std::byte{0x35}, "Repeated physical pages do not share storage");
        Require(contiguous->GetPage(1)[4] == std::byte{0x35}, "Separate leases do not share storage");

        auto* mapped = XMemVirtualAlloc(nullptr, AnyBox::PhysicalPageSize, 0x20402000, 0x23, 0x2004);
        XMemMapPhysicalPages(mapped, 1, frames.data() + 1);
        auto* bytes = static_cast<std::byte*>(mapped);
        Require(bytes[3] == std::byte{0x71} && bytes[4] == std::byte{0x35}, "Physical lease and guest mapping are not coherent");
        bytes[5] = std::byte{0x53};
        Require(scatter->GetPage(0)[5] == std::byte{0x53}, "Guest writes are not visible through physical leases");
        XMemMapPhysicalPages(mapped, 1, nullptr);
        Require(VirtualFree(mapped, 0, MEM_RELEASE) != 0, "Physical lease mapping window release failed");
    }
    Require(XMemFreePhysicalPages(frames.size(), frames.data()) != 0, "Released physical leases retained their pin counts");
    ExpectException<std::invalid_argument>([&] { Lease invalid(AnyBoxAcquirePhysicalPages(frames.size(), frames.data())); });
}

void CheckPinFailures() {
    using namespace AnyBox::Tests;
    std::size_t count = 2;
    std::array<std::uint64_t, 2> frames{};
    XMemAllocatePhysicalPages(0, &count, frames.data());
    std::array<std::uint64_t, 2> invalid{frames[0], std::numeric_limits<std::uint64_t>::max()};
    ExpectException<std::invalid_argument>([&] { Lease rejected(AnyBoxAcquirePhysicalPages(invalid.size(), invalid.data())); });
    {
        Lease first(AnyBoxAcquirePhysicalPages(1, frames.data()));
        Require(XMemFreePhysicalPages(1, frames.data() + 1) != 0, "A lease retained an unrelated physical page");
        Require(first->GetPage(0).size() == AnyBox::PhysicalPageSize, "Releasing an adjacent page invalidated a lease");
        first->GetPage(0)[0] = std::byte{0x57};
    }
    Require(XMemFreePhysicalPages(1, frames.data()) != 0, "Rejected physical pin leaked a valid frame reference");
    ExpectException<std::invalid_argument>([] { Lease rejected(AnyBoxAcquirePhysicalPages(0, nullptr)); });
    ExpectException<std::invalid_argument>([] { Lease rejected(AnyBoxAcquirePhysicalPages(1, nullptr)); });
}

}

void CheckMappedLeases();

int main() {
    try {
        CheckLeases();
        CheckPinFailures();
        CheckMappedLeases();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
