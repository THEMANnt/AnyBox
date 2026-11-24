#ifndef ANYBOX_XMEM_ALLOCATION_HEAP_HPP
#define ANYBOX_XMEM_ALLOCATION_HEAP_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace AnyBox::XMem {

class Heap {
public:
    ~Heap();
    void* Allocate(std::size_t size, std::uint64_t attributes, std::size_t alignment);
    void Free(void* allocation, std::uint64_t attributes);

private:
    struct Allocation {
        std::size_t alignment;
        std::uint64_t attributes;
    };

    std::mutex mutex;
    std::unordered_map<void*, Allocation> allocations;
};

Heap& GetHeap();

}

#endif
