#ifndef ANYBOX_XMEM_STATISTICS_HPP
#define ANYBOX_XMEM_STATISTICS_HPP

#include <cstdint>

namespace AnyBox::XMem {

std::uint32_t GetPageTableSize();
std::uint32_t GetStackSize();

}

#endif
