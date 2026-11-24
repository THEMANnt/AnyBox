#include "Lease.hpp"

#include <stdexcept>
#include <utility>

AnyBox::XMem::PhysicalPageLease::PhysicalPageLease(Pool& pool, std::vector<Page> retained, std::uintptr_t mappingBase) : pool(pool), retained(std::move(retained)), mappingBase(mappingBase) {}

AnyBox::XMem::PhysicalPageLease::~PhysicalPageLease() {
    if (active) pool.Release(retained, mappingBase);
}

void AnyBox::XMem::PhysicalPageLease::Activate() noexcept { active = true; }
std::size_t AnyBox::XMem::PhysicalPageLease::GetPageCount() const noexcept { return retained.size(); }

std::span<std::byte> AnyBox::XMem::PhysicalPageLease::GetPage(std::size_t index) const {
    if (index >= retained.size()) throw std::out_of_range("Physical page lease index exceeds its page count");
    const auto& page = retained[index];
    return {page.section->memory + page.offset, PhysicalPageSize};
}

std::span<std::byte> AnyBox::XMem::PhysicalPageLease::GetContiguousMemory() const noexcept {
    const auto& first = retained.front();
    for (std::size_t index = 1; index < retained.size(); ++index) {
        const auto& page = retained[index];
        if (page.section != first.section || page.offset != first.offset + index * PhysicalPageSize) return {};
    }
    return {first.section->memory + first.offset, retained.size() * PhysicalPageSize};
}

void AnyBox::XMem::PhysicalPageLease::Release() noexcept { delete this; }
