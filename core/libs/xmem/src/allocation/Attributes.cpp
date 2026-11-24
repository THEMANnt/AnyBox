#include "Attributes.hpp"

#include <algorithm>
#include <stdexcept>

AnyBox::XMem::AllocationAttributes AnyBox::XMem::DecodeAttributes(std::uint64_t attributes) {
    if ((attributes >> 34) != 0) throw std::invalid_argument("Unsupported XMem allocation attribute bits");
    auto pageSize = static_cast<std::uint32_t>((attributes >> 14) & 3);
    auto memoryType = static_cast<std::uint32_t>((attributes >> 29) & 15);
    if (pageSize == 3 || memoryType > 9) throw std::invalid_argument("Invalid XMem allocation memory type or page size");
    auto exponent = static_cast<std::uint32_t>((attributes >> 24) & 31);
    auto alignment = std::max<std::size_t>(16, std::size_t{1} << exponent);
    return {alignment, pageSize, memoryType};
}
