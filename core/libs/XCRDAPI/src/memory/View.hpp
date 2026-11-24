#ifndef ANYBOX_STORAGE_MEMORY_VIEW_HPP
#define ANYBOX_STORAGE_MEMORY_VIEW_HPP

#include <anybox/PhysicalMemory.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace AnyBox::Storage {

struct ReleasePhysicalPages {
    void operator()(IPhysicalPageLease* pages) const noexcept { pages->Release(); }
};

class MemoryView {
public:
    MemoryView() = default;
    MemoryView(void* memory, std::size_t size, bool physical, std::uint16_t pageOffset = 0);
    MemoryView(MemoryView&&) noexcept = default;
    MemoryView& operator=(MemoryView&&) noexcept = default;
    MemoryView(const MemoryView&) = delete;
    MemoryView& operator=(const MemoryView&) = delete;
    std::size_t GetSize() const noexcept;
    bool IsPhysical() const noexcept;
    std::span<std::byte> GetContiguousMemory() const noexcept;
    std::span<std::byte> GetChunk(std::size_t position) const;
    bool MayOverlap(const MemoryView& other) const;

private:
    std::unique_ptr<IPhysicalPageLease, ReleasePhysicalPages> pages;
    std::span<std::byte> contiguous;
    std::size_t size = 0;
    std::size_t pageOffset = 0;
    std::uintptr_t lower = 0;
    std::uintptr_t upper = 0;
    bool physical = false;
};

bool IsMappedMemory(std::span<std::byte> memory);

}

#endif
