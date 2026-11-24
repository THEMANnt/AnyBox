#include "Heap.hpp"

#include <AcpHal.hpp>

#include <stdexcept>

std::int32_t ApuAlloc(void** allocation, std::uint32_t* apuAddress, std::uint32_t size, std::uint32_t alignment, std::uint32_t flags) noexcept(false) {
    if (!allocation) throw std::invalid_argument("APU allocation output is null");
    *allocation = nullptr;
    if (apuAddress) *apuAddress = 0;
    auto* result = AnyBox::AcpHal::GetHeap().Allocate(size, alignment, flags);
    if (apuAddress) *apuAddress = AnyBox::AcpHal::GetHeap().ToApu(result);
    *allocation = result;
    return 0;
}

std::int32_t ApuCreateHeap(std::uint32_t normalSize, std::uint32_t shapeSize) noexcept(false) {
    AnyBox::AcpHal::GetHeap().Create(normalSize, shapeSize);
    return 0;
}

void ApuFree(void* allocation) noexcept(false) {
    AnyBox::AcpHal::GetHeap().Free(allocation);
}

bool ApuIsVirtualAddressValid(const void* address, std::uint32_t alignment) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().IsValid(address, alignment);
}

void* ApuMapApuAddress(std::uint32_t apuAddress) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().ToVirtual(apuAddress);
}

std::uint32_t ApuMapVirtualAddress(const void* address) noexcept(false) {
    return AnyBox::AcpHal::GetHeap().ToApu(address);
}
