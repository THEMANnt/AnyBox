#include "Pool.hpp"

#include <new>

AnyBox::AcpHal::Pool::Pool(std::uint32_t size, std::uint32_t busBase) : memory(static_cast<std::byte*>(::operator new(size, std::align_val_t{0x10000}))), memorySize(size), busBase(busBase), blocks{{0, size, false}} {}

void AnyBox::AcpHal::Pool::ReleaseMemory::operator()(std::byte* address) const noexcept {
    ::operator delete(address, std::align_val_t{0x10000});
}

bool AnyBox::AcpHal::Pool::Contains(const void* address) const noexcept {
    auto value = reinterpret_cast<std::uintptr_t>(address);
    auto base = reinterpret_cast<std::uintptr_t>(memory.get());
    return value >= base && value - base < memorySize;
}

bool AnyBox::AcpHal::Pool::Contains(std::uint32_t address) const noexcept {
    return address >= busBase && address - busBase < memorySize;
}
