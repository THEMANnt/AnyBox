#include "Heap.hpp"
#include <stdexcept>

std::span<std::byte> AnyBox::AcpHal::Pool::GetMemory(std::uint32_t address, std::size_t size) const {
    std::lock_guard lock(mutex);
    if (!Contains(address)) throw std::invalid_argument("APU memory range starts outside the pool");
    auto offset = address - busBase;
    for (const auto& block : blocks) {
        if (!block.allocated || offset < block.offset || offset - block.offset >= block.size) continue;
        if (size > block.size - (offset - block.offset)) throw std::length_error("APU memory range exceeds its allocation");
        return {memory.get() + offset, size};
    }
    throw std::invalid_argument("APU memory range does not belong to a live allocation");
}

std::span<std::byte> AnyBox::AcpHal::Heap::GetMemory(std::uint32_t address, std::size_t size) const {
    if (!created.load(std::memory_order_acquire)) throw std::logic_error("APU heap has not been created");
    for (const auto& pool : pools) if (pool->Contains(address)) return pool->GetMemory(address, size);
    throw std::invalid_argument("APU memory range is outside the heap");
}
