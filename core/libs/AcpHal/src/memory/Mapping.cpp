#include "Pool.hpp"

#include <bit>
#include <stdexcept>

bool AnyBox::AcpHal::Pool::IsValid(const void* address, std::uint32_t alignment) const {
    if (alignment == 0) alignment = 1;
    std::lock_guard lock(mutex);
    auto* base = memory.get();
    if (!base || !address) return false;
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto baseValue = reinterpret_cast<std::uintptr_t>(base);
    if (value < baseValue || value - baseValue >= memorySize) return false;
    auto offset = value - baseValue;
    for (const auto& block : blocks) {
        if (block.allocated && offset >= block.offset && offset - block.offset < block.size) {
            return ((busBase + offset) & (alignment - 1)) == 0;
        }
    }
    return false;
}

std::uint32_t AnyBox::AcpHal::Pool::ToApu(const void* address) const {
    if (!address) return 0;
    std::lock_guard lock(mutex);
    auto* base = memory.get();
    if (!base) throw std::logic_error("APU heap has not been created");
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto baseValue = reinterpret_cast<std::uintptr_t>(base);
    if (value < baseValue || value - baseValue >= memorySize) throw std::invalid_argument("CPU address is outside the APU heap");
    auto offset = value - baseValue;
    for (const auto& block : blocks) {
        if (block.allocated && offset >= block.offset && offset - block.offset < block.size) return busBase + static_cast<std::uint32_t>(offset);
    }
    throw std::invalid_argument("CPU address does not belong to a live APU allocation");
}

void* AnyBox::AcpHal::Pool::ToVirtual(std::uint32_t address) const {
    if (address == 0) return nullptr;
    std::lock_guard lock(mutex);
    auto* base = memory.get();
    if (!base) throw std::logic_error("APU heap has not been created");
    if (address < busBase || address - busBase >= memorySize) throw std::invalid_argument("APU address is outside the heap");
    auto offset = address - busBase;
    for (const auto& block : blocks) {
        if (block.allocated && offset >= block.offset && offset - block.offset < block.size) return base + offset;
    }
    throw std::invalid_argument("APU address does not belong to a live allocation");
}
