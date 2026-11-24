#include "Heap.hpp"

#include <stdexcept>

void AnyBox::AcpHal::Heap::initialize(std::uint32_t normalSize, std::uint32_t shapeSize) {
    if (normalSize == 0) throw std::invalid_argument("APU normal pool size must be nonzero");
    auto secondSize = static_cast<std::uint64_t>(shapeSize ? shapeSize : 0x114000) + 0x2000;
    auto secondBase = (0x10000ULL + normalSize + 0xFFFF) & ~0xFFFFULL;
    if (secondBase + secondSize > 0x100000000ULL) throw std::length_error("APU pools exceed their 32-bit address space");
    auto normal = std::make_unique<Pool>(normalSize, 0x10000);
    auto shape = std::make_unique<Pool>(static_cast<std::uint32_t>(secondSize), static_cast<std::uint32_t>(secondBase));
    pools[0] = std::move(normal);
    pools[1] = std::move(shape);
    created.store(true, std::memory_order_release);
}

void AnyBox::AcpHal::Heap::Create(std::uint32_t normalSize, std::uint32_t shapeSize) {
    std::lock_guard lock(mutex);
    if (created.load(std::memory_order_relaxed)) throw std::logic_error("APU heap already exists");
    initialize(normalSize, shapeSize);
}

void AnyBox::AcpHal::Heap::Initialize() {
    if (created.load(std::memory_order_acquire)) return;
    std::lock_guard lock(mutex);
    if (!created.load(std::memory_order_relaxed)) initialize(0x4000000, 0x114000);
}

AnyBox::AcpHal::Heap& AnyBox::AcpHal::GetHeap() {
    static Heap heap;
    return heap;
}
