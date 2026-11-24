#include "Attributes.hpp"
#include "Heap.hpp"

#include <xmem.hpp>

#include <algorithm>

void* XMemAllocDefault(std::size_t size, std::uint64_t attributes) noexcept(false) {
    if (size == 0) return nullptr;
    auto decoded = AnyBox::XMem::DecodeAttributes(attributes);
    if (decoded.MemoryType != 0) {
        constexpr std::size_t pageSizes[]{4096, 65536, 2097152};
        decoded.Alignment = std::max(decoded.Alignment, pageSizes[decoded.PageSize]);
    }
    return AnyBox::XMem::GetHeap().Allocate(size, attributes, decoded.Alignment);
}

void XMemFreeDefault(void* allocation, std::uint64_t attributes) noexcept(false) {
    AnyBox::XMem::DecodeAttributes(attributes);
    AnyBox::XMem::GetHeap().Free(allocation, attributes);
}
