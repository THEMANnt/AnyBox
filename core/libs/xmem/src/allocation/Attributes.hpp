#ifndef ANYBOX_XMEM_ALLOCATION_ATTRIBUTES_HPP
#define ANYBOX_XMEM_ALLOCATION_ATTRIBUTES_HPP

#include <cstddef>
#include <cstdint>

namespace AnyBox::XMem {

struct AllocationAttributes {
    std::size_t Alignment;
    std::uint32_t PageSize;
    std::uint32_t MemoryType;
};

AllocationAttributes DecodeAttributes(std::uint64_t attributes);

}

#endif
