#include "Heap.hpp"

#include <stdexcept>

AnyBox::AcpHal::Pool& AnyBox::AcpHal::Heap::getPool(const void* address) const {
    if (!created.load(std::memory_order_acquire)) throw std::logic_error("APU heap has not been created");
    for (const auto& pool : pools) if (pool->Contains(address)) return *pool;
    throw std::invalid_argument("CPU address is outside the APU pools");
}

void* AnyBox::AcpHal::Heap::Allocate(std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) {
    if (flags < 1 || flags > 2) throw std::invalid_argument("Unsupported APU allocation flags");
    Initialize();
    return pools[flags - 1]->Allocate(size, alignment);
}

void AnyBox::AcpHal::Heap::Free(void* allocation) {
    if (allocation) getPool(allocation).Free(allocation);
}

bool AnyBox::AcpHal::Heap::IsValid(const void* address, std::uint32_t alignment) const {
    if (!address || !created.load(std::memory_order_acquire)) return false;
    for (const auto& pool : pools) if (pool->Contains(address)) return pool->IsValid(address, alignment);
    return false;
}

std::uint32_t AnyBox::AcpHal::Heap::ToApu(const void* address) const {
    return address ? getPool(address).ToApu(address) : 0;
}

void* AnyBox::AcpHal::Heap::ToVirtual(std::uint32_t address) const {
    if (address == 0) return nullptr;
    if (!created.load(std::memory_order_acquire)) throw std::logic_error("APU heap has not been created");
    for (const auto& pool : pools) if (pool->Contains(address)) return pool->ToVirtual(address);
    throw std::invalid_argument("APU address is outside the pools");
}
