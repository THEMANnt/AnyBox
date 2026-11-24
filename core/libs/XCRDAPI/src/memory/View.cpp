#include "View.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

AnyBox::Storage::MemoryView::MemoryView(void* memory, std::size_t size, bool physical, std::uint16_t pageOffset) : size(size), pageOffset(pageOffset), physical(physical) {
    if (size == 0) return;
    if (!memory) throw std::invalid_argument("DirectStorage memory range is null");
    if (!physical) {
        lower = reinterpret_cast<std::uintptr_t>(memory);
        if (size > std::numeric_limits<std::uintptr_t>::max() - lower) throw std::overflow_error("DirectStorage memory range overflows");
        upper = lower + size;
        contiguous = {static_cast<std::byte*>(memory), size};
        return;
    }
    if (size > std::numeric_limits<std::size_t>::max() - pageOffset - (PhysicalPageSize - 1)) throw std::overflow_error("DirectStorage physical page count overflows");
    auto count = (size + pageOffset + PhysicalPageSize - 1) / PhysicalPageSize;
    auto array = reinterpret_cast<std::uintptr_t>(memory);
    if (count > (std::numeric_limits<std::uintptr_t>::max() - array) / sizeof(std::uint64_t)) throw std::overflow_error("DirectStorage physical page array overflows");
    pages.reset(AnyBoxAcquirePhysicalPages(count, static_cast<const std::uint64_t*>(memory)));
    if (!pages) throw std::runtime_error("Physical memory provider returned a null lease");
    auto range = pages->GetContiguousMemory();
    if (!range.empty()) {
        contiguous = range.subspan(pageOffset, size);
        lower = reinterpret_cast<std::uintptr_t>(contiguous.data());
        upper = lower + size;
        return;
    }
    lower = std::numeric_limits<std::uintptr_t>::max();
    for (std::size_t index = 0; index < pages->GetPageCount(); ++index) {
        auto page = pages->GetPage(index);
        auto address = reinterpret_cast<std::uintptr_t>(page.data());
        lower = std::min(lower, address);
        upper = std::max(upper, address + page.size());
    }
}

std::size_t AnyBox::Storage::MemoryView::GetSize() const noexcept { return size; }
bool AnyBox::Storage::MemoryView::IsPhysical() const noexcept { return physical; }
std::span<std::byte> AnyBox::Storage::MemoryView::GetContiguousMemory() const noexcept { return contiguous; }

std::span<std::byte> AnyBox::Storage::MemoryView::GetChunk(std::size_t position) const {
    if (position > size) throw std::out_of_range("DirectStorage memory position exceeds its range");
    if (position == size) return {};
    if (!contiguous.empty()) return contiguous.subspan(position);
    auto absolute = pageOffset + position;
    auto offset = absolute % PhysicalPageSize;
    return pages->GetPage(absolute / PhysicalPageSize).subspan(offset, std::min(size - position, PhysicalPageSize - offset));
}

bool AnyBox::Storage::MemoryView::MayOverlap(const MemoryView& other) const {
    if (size == 0 || other.size == 0) return false;
    if (lower < other.upper && other.lower < upper) return true;
    if (physical == other.physical) return false;
    return IsMappedMemory(physical ? other.contiguous : contiguous);
}
