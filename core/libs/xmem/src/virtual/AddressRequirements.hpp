#ifndef ANYBOX_XMEM_ADDRESS_REQUIREMENTS_HPP
#define ANYBOX_XMEM_ADDRESS_REQUIREMENTS_HPP

#include <anybox/platform/Windows.hpp>

#include <cstdint>

namespace AnyBox::XMem {

MEM_ADDRESS_REQUIREMENTS GetAddressRequirements(std::uint32_t allocationType, std::uint64_t attributes);

}

#endif
