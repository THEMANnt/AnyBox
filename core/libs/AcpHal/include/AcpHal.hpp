#ifndef ANYBOX_ACPHAL_HPP
#define ANYBOX_ACPHAL_HPP

#include <AcpHalTypes.hpp>

#include <cstdint>

extern "C" {

std::int32_t AcpHalAllocateShapeContexts(AcpHalShapeContexts* shapeContexts) noexcept(false);
std::int32_t AcpHalCreate(IAcpHal** acpHal) noexcept(false);
std::int32_t AcpHalReleaseShapeContexts() noexcept(false);
std::int32_t ApuAlloc(void** allocation, std::uint32_t* apuAddress, std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) noexcept(false);
std::int32_t ApuCreateHeap(std::uint32_t normalSize, std::uint32_t shapeSize) noexcept(false);
void ApuFree(void* allocation) noexcept(false);
bool ApuIsVirtualAddressValid(const void* address, std::uint32_t alignment) noexcept(false);
void* ApuMapApuAddress(std::uint32_t apuAddress) noexcept(false);
std::uint32_t ApuMapVirtualAddress(const void* address) noexcept(false);

}

#endif
