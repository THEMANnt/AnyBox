#ifndef ANYBOX_XMEM_VIRTUAL_VALIDATION_HPP
#define ANYBOX_XMEM_VIRTUAL_VALIDATION_HPP

#include <cstdint>

namespace AnyBox::XMem {

void ValidateVirtualAllocation(std::uint32_t allocationType, std::uint64_t attributes, std::uint32_t protection);

}

#endif
