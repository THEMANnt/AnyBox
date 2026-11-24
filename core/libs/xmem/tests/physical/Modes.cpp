#include "Tests.hpp"

#include <anybox/PhysicalMemory.hpp>

#include <array>
#include <limits>

void AnyBox::Tests::CheckPhysicalPageModes() {
    std::size_t empty = 0;
    Require(XMemAllocatePhysicalPages(0, &empty, nullptr) == 1 && empty == 0, "Empty physical allocation failed");
    std::uint64_t sentinel = 0x173A8D57;
    Require(XMemAllocatePhysicalPages(0x20000000, &empty, &sentinel) == 1 && sentinel == 0x173A8D57, "Empty physical allocation changed its output");
    auto* unused = reinterpret_cast<void*>(std::numeric_limits<std::uintptr_t>::max());
    Require(XMemMapPhysicalPages(unused, 0, nullptr) == unused, "Empty physical mapping touched its address");
    ExpectException<std::invalid_argument>([] { XMemMapPhysicalPages(nullptr, 0, nullptr); });
    std::array<std::uint64_t, 32> frames{};
    std::size_t count = 31;
    ExpectException<std::invalid_argument>([&] { XMemAllocatePhysicalPages(0x20000000, &count, frames.data()); });
    Require(count == 31 && frames.front() == 0 && frames.back() == 0, "Rejected large physical allocation changed outputs");

    count = frames.size();
    Require(XMemAllocatePhysicalPages(0x20000280, &count, frames.data()) == 1 && count == frames.size(), "Large physical allocation failed");
    Require(frames.front() % 512 == 0, "Large physical allocation is not aligned to 2 MiB");
    for (std::size_t index = 1; index < frames.size(); ++index) Require(frames[index] == frames.front() + index * 16, "Large physical page identifiers are not contiguous");
    auto* lease = AnyBoxAcquirePhysicalPages(count, frames.data());
    auto memory = lease->GetContiguousMemory();
    Require(memory.size() == 0x200000, "Large physical allocation backing is not contiguous");
    memory.front() = std::byte{0x37};
    memory.back() = std::byte{0x59};
    auto* window = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, memory.size(), 0x20002000, 0x21, PAGE_READWRITE));
    Require(reinterpret_cast<std::uintptr_t>(window) % 0x200000 == 0, "Large physical mapping window is misaligned");
    XMemMapPhysicalPages(window, count, frames.data());
    Require(window[0] == std::byte{0x37} && window[0x1FFFFF] == std::byte{0x59}, "Large physical mapping does not share its backing");
    window[0xFFFF] = std::byte{0x73};
    Require(memory[0xFFFF] == std::byte{0x73}, "Large physical mapping writes are not coherent");
    lease->Release();
    XMemMapPhysicalPages(window, count, nullptr);
    Require(XMemFreePhysicalPages(count, frames.data()) == 1, "Large physical allocation did not release");
    for (std::size_t index = 0; index < frames.size(); ++index) {
        Require(VirtualFree(window + index * AnyBox::PhysicalPageSize, 0, MEM_RELEASE) != 0, "Large physical mapping placeholder did not release");
    }
}
