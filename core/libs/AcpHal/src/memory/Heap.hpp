#ifndef ANYBOX_ACPHAL_MEMORY_HEAP_HPP
#define ANYBOX_ACPHAL_MEMORY_HEAP_HPP

#include "Pool.hpp"

#include <array>
#include <atomic>

namespace AnyBox::AcpHal {

class Heap {
public:
    void Create(std::uint32_t normalSize, std::uint32_t shapeSize);
    void Initialize();
    void* Allocate(std::uint32_t size, std::uint32_t alignment, std::uint32_t flags);
    void Free(void* allocation);
    bool IsValid(const void* address, std::uint32_t alignment) const;
    std::uint32_t ToApu(const void* address) const;
    void* ToVirtual(std::uint32_t address) const;
    std::span<std::byte> GetMemory(std::uint32_t address, std::size_t size) const;

private:
    void initialize(std::uint32_t normalSize, std::uint32_t shapeSize);
    Pool& getPool(const void* address) const;
    mutable std::mutex mutex;
    std::atomic<bool> created{false};
    std::array<std::unique_ptr<Pool>, 2> pools;
};

Heap& GetHeap();

}

#endif
