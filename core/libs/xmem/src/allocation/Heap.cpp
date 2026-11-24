#include "Heap.hpp"
#include "Storage.hpp"

#include <new>
#include <stdexcept>

AnyBox::XMem::Heap::~Heap() {
    for (const auto& [address, allocation] : allocations) {
        FreeHeapStorage(address, allocation.attributes, allocation.alignment);
    }
}

void* AnyBox::XMem::Heap::Allocate(std::size_t size, std::uint64_t attributes, std::size_t alignment) {
    auto* allocation = AllocateHeapStorage(size, attributes, alignment);
    try {
        std::lock_guard lock(mutex);
        allocations.emplace(allocation, Allocation{alignment, attributes});
    } catch (...) {
        FreeHeapStorage(allocation, attributes, alignment);
        throw;
    }
    return allocation;
}

void AnyBox::XMem::Heap::Free(void* allocation, std::uint64_t attributes) {
    if (!allocation) return;
    std::size_t alignment;
    {
        std::lock_guard lock(mutex);
        auto it = allocations.find(allocation);
        if (it == allocations.end()) throw std::invalid_argument("Address is not a live XMem heap allocation");
        if (it->second.attributes != attributes) throw std::invalid_argument("XMem free attributes differ from allocation attributes");
        alignment = it->second.alignment;
        allocations.erase(it);
    }
    FreeHeapStorage(allocation, attributes, alignment);
}

AnyBox::XMem::Heap& AnyBox::XMem::GetHeap() {
    static Heap heap;
    return heap;
}
