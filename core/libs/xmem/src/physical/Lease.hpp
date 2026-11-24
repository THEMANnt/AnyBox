#ifndef ANYBOX_XMEM_PHYSICAL_LEASE_HPP
#define ANYBOX_XMEM_PHYSICAL_LEASE_HPP

#include "Pool.hpp"

namespace AnyBox::XMem {

class PhysicalPageLease final : public IPhysicalPageLease {
public:
    PhysicalPageLease(Pool& pool, std::vector<Page> retained, std::uintptr_t mappingBase);
    ~PhysicalPageLease();
    void Activate() noexcept;
    std::size_t GetPageCount() const noexcept override;
    std::span<std::byte> GetPage(std::size_t index) const override;
    std::span<std::byte> GetContiguousMemory() const noexcept override;
    void Release() noexcept override;

private:
    Pool& pool;
    std::vector<Page> retained;
    std::uintptr_t mappingBase;
    bool active = false;
};

}

#endif
