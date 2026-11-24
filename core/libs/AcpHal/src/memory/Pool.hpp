#ifndef ANYBOX_ACPHAL_MEMORY_POOL_HPP
#define ANYBOX_ACPHAL_MEMORY_POOL_HPP

#include <cstddef>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <span>

namespace AnyBox::AcpHal {

class Pool {
public:
    Pool(std::uint32_t size, std::uint32_t busBase);
    void* Allocate(std::uint32_t size, std::uint32_t alignment);
    void Free(void* allocation);
    bool Contains(const void* address) const noexcept;
    bool Contains(std::uint32_t address) const noexcept;
    bool IsValid(const void* address, std::uint32_t alignment) const;
    std::uint32_t ToApu(const void* address) const;
    void* ToVirtual(std::uint32_t address) const;
    std::span<std::byte> GetMemory(std::uint32_t address, std::size_t size) const;

private:
    struct Block {
        std::size_t offset;
        std::size_t size;
        bool allocated;
    };

    struct ReleaseMemory {
        void operator()(std::byte* address) const noexcept;
    };

    mutable std::mutex mutex;
    std::unique_ptr<std::byte, ReleaseMemory> memory;
    std::uint32_t memorySize;
    std::uint32_t busBase;
    std::list<Block> blocks;
};

}

#endif
