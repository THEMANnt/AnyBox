#ifndef ANYBOX_ACPHAL_TYPES_HPP
#define ANYBOX_ACPHAL_TYPES_HPP

#include <array>
#include <cstddef>
#include <cstdint>

struct IAcpHal;

struct AcpHalShapeContexts {
    std::array<std::uint32_t, 6> counts;
    std::array<void*, 6> contexts;
    std::array<std::uint32_t, 6> apuAddresses;
};

static_assert(offsetof(AcpHalShapeContexts, contexts) == 24);
static_assert(offsetof(AcpHalShapeContexts, apuAddresses) == 72);
static_assert(sizeof(AcpHalShapeContexts) == 96);

#endif
