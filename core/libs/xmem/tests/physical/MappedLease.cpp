#include "../virtual/Tests.hpp"
#include <anybox/PhysicalMemory.hpp>
#include <array>
#include <memory>

namespace {
struct ReleaseLease {
    void operator()(AnyBox::IPhysicalPageLease* value) const noexcept { value->Release(); }
};
using Lease = std::unique_ptr<AnyBox::IPhysicalPageLease, ReleaseLease>;
}

void CheckMappedLeases() {
    using namespace AnyBox::Tests;
    constexpr auto page = AnyBox::PhysicalPageSize;
    std::array<std::uint64_t, 3> frames{};
    auto count = frames.size();
    XMemAllocatePhysicalPages(0x280, &count, frames.data());
    auto* address = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, 3 * page, 0x20402000, 0x23, 0x2004));
    XMemMapPhysicalPages(address, 3, frames.data());
    {
        Lease first(AnyBoxAcquireMappedPhysicalPages(address + page, 2));
        Lease second(AnyBoxAcquireMappedPhysicalPages(address + 2 * page, 1));
        auto memory = first->GetContiguousMemory();
        Require(memory.size() == 2 * page, "Mapped physical lease lost a contiguous backing range");
        address[page + 17] = std::byte{0x73};
        Require(memory[17] == std::byte{0x73}, "Mapped lease does not refer to its title address");
        memory[page + 18] = std::byte{0x27};
        Require(address[2 * page + 18] == std::byte{0x27}, "Mapped lease writes did not reach the title address");
        ExpectException<std::logic_error>([&] { XMemMapPhysicalPages(address + page, 2, nullptr); });
        ExpectException<std::logic_error>([&] { XMemMapPhysicalPages(address + page, 1, frames.data()); });
        Require(XMemMapPhysicalPages(address + page, 2, frames.data() + 1) == address + page, "Identical mapping changed a pinned GPU range");
        first.reset();
        XMemMapPhysicalPages(address + page, 1, nullptr);
        ExpectException<std::logic_error>([&] { XMemMapPhysicalPages(address + 2 * page, 1, nullptr); });
        second.reset();
        XMemMapPhysicalPages(address + 2 * page, 1, nullptr);
    }
    ExpectException<std::invalid_argument>([&] { Lease rejected(AnyBoxAcquireMappedPhysicalPages(address, 2)); });
    XMemMapPhysicalPages(address + page, 2, frames.data() + 1);
    XMemMapPhysicalPages(address + 2 * page, 1, frames.data());
    {
        Lease scatter(AnyBoxAcquireMappedPhysicalPages(address + page, 2));
        Require(scatter->GetContiguousMemory().empty(), "Reordered mapped pages were exposed as contiguous memory");
        Require(scatter->GetPageCount() == 2, "Mapped scatter lease changed its page count");
    }
    ExpectException<std::invalid_argument>([&] { Lease rejected(AnyBoxAcquireMappedPhysicalPages(address + 1, 1)); });
    ExpectException<std::invalid_argument>([&] { Lease rejected(AnyBoxAcquireMappedPhysicalPages(nullptr, 1)); });
    ExpectException<std::invalid_argument>([&] { Lease rejected(AnyBoxAcquireMappedPhysicalPages(address, 0)); });
    XMemMapPhysicalPages(address, 3, nullptr);
    for (std::size_t index = 0; index < frames.size(); ++index) Require(VirtualFree(address + index * page, 0, MEM_RELEASE) != FALSE, "Mapped lease test placeholder did not release");
    XMemFreePhysicalPages(frames.size(), frames.data());
}
