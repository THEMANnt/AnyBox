#include "Tests.hpp"

#include <cstddef>
#include <cstdint>

void AnyBox::Tests::CheckMapping() {
    void* allocation = nullptr;
    Require(ApuAlloc(&allocation, nullptr, 65536, 65536, 2) == 0, "APU mapping allocation failed");
    auto address = ApuMapVirtualAddress(allocation);
    Require(address != 0 && address % 65536 == 0, "APU bus alignment changed");
    auto* bytes = static_cast<std::byte*>(allocation);
    for (std::uint32_t i = 0; i < 65536; ++i) {
        Require(ApuMapVirtualAddress(bytes + i) == address + i, "APU translation is not affine");
        Require(ApuMapApuAddress(address + i) == bytes + i, "APU translation did not round-trip");
        Require(ApuIsVirtualAddressValid(bytes + i, 64) == (i % 64 == 0), "APU validity ignored alignment");
    }
    *static_cast<std::byte*>(ApuMapApuAddress(address + 123)) = std::byte{0x7B};
    Require(bytes[123] == std::byte{0x7B}, "APU address did not reference CPU storage");
    ApuFree(allocation);
    Require(!ApuIsVirtualAddressValid(allocation, 0), "Freed APU storage is still valid");
    ExpectException<std::invalid_argument>([&] { ApuMapVirtualAddress(allocation); });
    ExpectException<std::invalid_argument>([&] { ApuMapApuAddress(address); });
}
