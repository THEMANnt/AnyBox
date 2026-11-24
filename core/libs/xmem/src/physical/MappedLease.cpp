#include "Pool.hpp"
#include <limits>
#include <stdexcept>

AnyBox::IPhysicalPageLease* AnyBox::XMem::Pool::AcquireMapped(std::uintptr_t address, std::size_t count) {
    if (address == 0 || address % PhysicalPageSize != 0 || count == 0) throw std::invalid_argument("Mapped physical lease requires an aligned address and page count");
    if (count > (std::numeric_limits<std::uintptr_t>::max() - address) / PhysicalPageSize) throw std::overflow_error("Mapped physical lease address range overflows");
    std::scoped_lock lock(mutex);
    std::vector<Page> retained;
    retained.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        auto mapping = mappings.find(address + index * PhysicalPageSize);
        if (mapping == mappings.end()) throw std::invalid_argument("Mapped physical lease refers to an unmapped page");
        retained.push_back(mapping->second);
    }
    return retain(std::move(retained), address);
}

AnyBox::IPhysicalPageLease* AnyBoxAcquireMappedPhysicalPages(const void* address, std::size_t pageCount) noexcept(false) {
    return AnyBox::XMem::GetPool().AcquireMapped(reinterpret_cast<std::uintptr_t>(address), pageCount);
}
