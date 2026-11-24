#ifndef ANYBOX_PHYSICAL_MEMORY_HPP
#define ANYBOX_PHYSICAL_MEMORY_HPP

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox {

inline constexpr std::size_t PhysicalPageSize = 65536;

struct IPhysicalPageLease {
    virtual std::size_t GetPageCount() const noexcept = 0;
    virtual std::span<std::byte> GetPage(std::size_t index) const = 0;
    virtual std::span<std::byte> GetContiguousMemory() const noexcept = 0;
    virtual void Release() noexcept = 0;
};

}

extern "C" AnyBox::IPhysicalPageLease* AnyBoxAcquirePhysicalPages(std::size_t pageCount, const std::uint64_t* pageFrames) noexcept(false);
extern "C" AnyBox::IPhysicalPageLease* AnyBoxAcquireMappedPhysicalPages(const void* address, std::size_t pageCount) noexcept(false);

#endif
