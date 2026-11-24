#include "Pool.hpp"

#include <xmem.hpp>

#include <limits>
#include <stdexcept>

std::int32_t XMemAllocatePhysicalPages(std::uint64_t flags, std::size_t* pageCount, std::uint64_t* pageFrames) noexcept(false) {
    if (!pageCount) throw std::invalid_argument("XMem physical page count is null");
    AnyBox::XMem::GetPool().Allocate(flags, *pageCount, pageFrames);
    return 1;
}

std::int32_t XMemFreePhysicalPages(std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false) {
    if (pageCount != 0 && !pageFrames) throw std::invalid_argument("XMem physical free array is null");
    AnyBox::XMem::GetPool().Free({pageFrames, pageCount});
    return 1;
}

void* XMemMapPhysicalPages(void* address, std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false) {
    auto value = reinterpret_cast<std::uintptr_t>(address);
    if (!address) throw std::invalid_argument("XMem physical mapping requires an address");
    if (pageCount == 0) return address;
    if (pageCount > (std::numeric_limits<std::uintptr_t>::max() - value) / AnyBox::XMem::PhysicalPageSize) throw std::overflow_error("XMem physical mapping address range overflows");
    std::vector<void*> addresses;
    addresses.reserve(pageCount);
    for (std::size_t i = 0; i < pageCount; ++i) addresses.push_back(reinterpret_cast<void*>(value + i * AnyBox::XMem::PhysicalPageSize));
    AnyBox::XMem::GetPool().Map(addresses, pageFrames);
    return address;
}

std::int32_t XMemMapPhysicalPagesScatter(void* const* addresses, std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false) {
    if (!addresses) throw std::invalid_argument("XMem scatter address array is null");
    if (pageCount == 0) return 1;
    AnyBox::XMem::GetPool().Map({addresses, pageCount}, pageFrames);
    return 1;
}
