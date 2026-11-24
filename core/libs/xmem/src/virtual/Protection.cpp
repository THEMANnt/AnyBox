#include "Protection.hpp"

#include <anybox/platform/Windows.hpp>

#include <bit>
#include <stdexcept>

std::uint32_t AnyBox::XMem::HostProtection(std::uint32_t protection) {
    constexpr std::uint32_t graphicsMask = 0x7F800;
    constexpr std::uint32_t cpuMask = 0x7FF;
    if ((protection & ~(graphicsMask | cpuMask)) != 0) throw std::invalid_argument("Unsupported XMem protection bits");
    auto basic = protection & 0xFF;
    if (!std::has_single_bit(basic)) throw std::invalid_argument("Invalid XMem CPU page protection");
    if ((protection & PAGE_NOCACHE) != 0 && (protection & PAGE_WRITECOMBINE) != 0) {
        throw std::invalid_argument("Conflicting XMem CPU cache policies");
    }
    return protection & cpuMask;
}
