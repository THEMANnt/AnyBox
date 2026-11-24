#include "Tests.hpp"

#include <cstddef>
#include <cstring>
#include <iostream>
#include <string_view>

int main(int argumentCount, char** arguments) {
    try {
        using AnyBox::Tests::Require;
        if (argumentCount == 2 && std::string_view(arguments[1]) == "--lazy-shape") {
            AcpHalShapeContexts contexts{};
            Require(AcpHalAllocateShapeContexts(&contexts) == 0, "Lazy SHAPE context initialization failed");
            AnyBox::Tests::ExpectException<std::logic_error>([] { ApuCreateHeap(0x100000, 0); });
            Require(AcpHalReleaseShapeContexts() == 0, "Lazy SHAPE contexts did not release");
            return 0;
        }
        bool lazy = argumentCount == 2 && std::string_view(arguments[1]) == "--lazy";
        bool defaultShape = argumentCount == 2 && std::string_view(arguments[1]) == "--default-shape";
        if (!lazy) Require(ApuCreateHeap(0x100000, defaultShape ? 0 : 0x226E00) == 0, "APU pools could not be created");
        void* allocation = nullptr;
        Require(ApuAlloc(&allocation, nullptr, 65536, 2048, 2) == 0, "APU allocation failed with independent backing");
        Require(ApuIsVirtualAddressValid(allocation, 2048), "APU default heap allocation is invalid");
        std::memset(allocation, 0x6D, 65536);
        auto* bytes = static_cast<std::byte*>(allocation);
        auto address = ApuMapVirtualAddress(allocation);
        for (std::uint32_t i = 0; i < 65536; ++i) {
            Require(ApuMapApuAddress(address + i) == bytes + i, "APU default heap translation failed");
            Require(bytes[i] == std::byte{0x6D}, "APU default heap contents changed");
        }
        ApuFree(allocation);
        if (lazy) AnyBox::Tests::ExpectException<std::logic_error>([] { ApuCreateHeap(0x100000, 0); });
        if (defaultShape) {
            Require(ApuAlloc(&allocation, nullptr, 0x116000, 256, 2) == 0, "Default SHAPE pool size changed");
            ApuFree(allocation);
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
