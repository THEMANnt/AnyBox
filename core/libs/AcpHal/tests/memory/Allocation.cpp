#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

void AnyBox::Tests::CheckAllocation() {
    std::array<void*, 3> allocations{};
    for (std::uint32_t i = 0; i < allocations.size(); ++i) {
        Require(ApuAlloc(&allocations[i], nullptr, 4096, 2048, i % 2 + 1) == 0, "APU allocation failed");
        Require(reinterpret_cast<std::uintptr_t>(allocations[i]) % 2048 == 0, "APU allocation is misaligned");
        std::memset(allocations[i], static_cast<int>(i + 1), 4096);
    }
    for (std::size_t i = 0; i < allocations.size(); ++i) {
        for (std::size_t j = i + 1; j < allocations.size(); ++j) Require(allocations[i] != allocations[j], "APU allocations overlap");
        auto* bytes = static_cast<const std::byte*>(allocations[i]);
        for (std::size_t j = 0; j < 4096; ++j) Require(bytes[j] == std::byte(i + 1), "APU allocation contents changed");
    }
    ApuFree(allocations[1]);
    void* replacement = nullptr;
    std::uint32_t apuAddress = 0;
    Require(ApuAlloc(&replacement, &apuAddress, 4096, 2048, 1) == 0, "APU allocation with bus output failed");
    Require(apuAddress != 0 && ApuMapApuAddress(apuAddress) == replacement, "APU allocation bus output does not map to its CPU output");
    ApuFree(allocations[0]);
    ApuFree(allocations[2]);
    ApuFree(replacement);
    void* entireHeap = nullptr;
    Require(ApuAlloc(&entireHeap, nullptr, 0xFFF00, 256, 1) == 0, "APU free blocks were not coalesced");
    std::memset(entireHeap, 0x5A, 0xFFF00);
    ApuFree(entireHeap);
    ApuFree(nullptr);

    void* normal = nullptr;
    void* shape = nullptr;
    void* overflow = nullptr;
    Require(ApuAlloc(&normal, nullptr, 0xFE000, 256, 1) == 0, "APU normal pool allocation failed");
    ExpectException<std::bad_alloc>([&] { ApuAlloc(&overflow, nullptr, 0x4000, 256, 1); });
    Require(ApuAlloc(&shape, nullptr, 0x180000, 256, 2) == 0, "Exhausted normal pool incorrectly blocked SHAPE allocation");
    std::memset(normal, 0x37, 0xFE000);
    std::memset(shape, 0x59, 0x180000);
    auto normalAddress = ApuMapVirtualAddress(normal);
    auto shapeAddress = ApuMapVirtualAddress(shape);
    Require(normalAddress + 0xFE000 <= shapeAddress || shapeAddress + 0x180000 <= normalAddress, "APU pools overlap in bus address space");
    Require(ApuMapApuAddress(normalAddress + 0xFDFFF) == static_cast<std::byte*>(normal) + 0xFDFFF, "Normal pool translation changed");
    Require(ApuMapApuAddress(shapeAddress + 0x17FFFF) == static_cast<std::byte*>(shape) + 0x17FFFF, "SHAPE pool translation changed");
    Require(static_cast<std::byte*>(normal)[0xFDFFF] == std::byte{0x37}, "SHAPE allocation overwrote the normal pool");
    ApuFree(normal);
    ApuFree(shape);

    for (auto alignment : {0x20000u, 0x40000u, 0x80000u}) {
        for (auto pool : {1u, 2u}) {
            void* allocation = nullptr;
            std::uint32_t address = 0;
            Require(ApuAlloc(&allocation, &address, 13, alignment, pool) == 0, "Large aligned APU allocation failed");
            Require(address % alignment == 0, "APU bus address does not meet the requested alignment");
            Require(ApuIsVirtualAddressValid(allocation, alignment), "Valid APU bus alignment was rejected");
            Require(ApuMapApuAddress(address + alignment - 1) == static_cast<std::byte*>(allocation) + alignment - 1, "Rounded APU allocation range is incomplete");
            std::memset(allocation, 0x71, alignment);
            ApuFree(allocation);
        }
    }
}
