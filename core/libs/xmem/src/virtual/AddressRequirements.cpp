#include "AddressRequirements.hpp"

MEM_ADDRESS_REQUIREMENTS AnyBox::XMem::GetAddressRequirements(std::uint32_t allocationType, std::uint64_t attributes) {
    MEM_ADDRESS_REQUIREMENTS result{};
    auto pageMode = allocationType & 0x20400000;
    result.Alignment = pageMode == 0x20000000 || (allocationType & MEM_RESERVE_PLACEHOLDER) != 0 ? 0x200000 : 0x10000;
    if ((attributes & 2) != 0) {
        bool address32 = (attributes & 4) != 0;
        result.LowestStartingAddress = reinterpret_cast<void*>(address32 ? 0x80000000ULL : 0x100000000ULL);
        result.HighestEndingAddress = reinterpret_cast<void*>(address32 ? 0xFFFFFFFFULL : 0xFFFFFFFFFFULL);
        return result;
    }
    auto addressBits = static_cast<unsigned>((attributes >> 12) & 7);
    if (addressBits == 0) return result;
    result.LowestStartingAddress = reinterpret_cast<void*>(addressBits == 7 ? 0x100000000ULL : 0x10000000000ULL);
    result.HighestEndingAddress = reinterpret_cast<void*>((1ULL << (47 - addressBits)) - 1);
    return result;
}
