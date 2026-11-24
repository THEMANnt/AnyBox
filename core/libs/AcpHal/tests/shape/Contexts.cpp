#include "../memory/Tests.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>

int main() {
    try {
        using namespace AnyBox::Tests;
        ApuCreateHeap(0x100000, 0x226E00);
        AcpHalShapeContexts descriptor{};
        Require(AcpHalAllocateShapeContexts(&descriptor) == 0, "Empty SHAPE descriptor was rejected");
        for (auto* context : descriptor.contexts) Require(context == nullptr, "Empty SHAPE array allocated storage");
        ExpectException<std::logic_error>([&] { AcpHalAllocateShapeContexts(&descriptor); });
        Require(AcpHalReleaseShapeContexts() == 0, "SHAPE release failed");
        descriptor.counts = {357, 13, 21, 9, 350, 7};
        Require(AcpHalAllocateShapeContexts(&descriptor) == 0, "SHAPE context allocation failed");
        for (std::size_t i = 0; i < 6; ++i) {
            Require(descriptor.contexts[i] != nullptr, "SHAPE context storage is missing");
            Require(ApuMapApuAddress(descriptor.apuAddresses[i]) == descriptor.contexts[i], "SHAPE bus address does not map to its CPU address");
        }
        auto* shared = static_cast<std::byte*>(descriptor.contexts[4]);
        Require(descriptor.contexts[5] == shared + 350 * 64, "SHAPE shared context layout changed");
        std::memset(shared, 0x73, 350 * 64 + 7 * 32);
        auto saved = descriptor.contexts;
        ExpectException<std::logic_error>([&] { AcpHalAllocateShapeContexts(&descriptor); });
        Require(descriptor.contexts == saved, "Duplicate SHAPE allocation changed its output");
        Require(AcpHalReleaseShapeContexts() == 0, "SHAPE release failed");
        for (auto* context : saved) Require(!ApuIsVirtualAddressValid(context, 0), "SHAPE release retained live context storage");
        descriptor.counts[0] = 358;
        ExpectException<std::invalid_argument>([&] { AcpHalAllocateShapeContexts(&descriptor); });
        descriptor.counts = {0, 0, 0, 0, 513, 0};
        ExpectException<std::invalid_argument>([&] { AcpHalAllocateShapeContexts(&descriptor); });
        descriptor.counts = {0, 4097, 0, 0, 0, 0};
        ExpectException<std::invalid_argument>([&] { AcpHalAllocateShapeContexts(&descriptor); });
        ExpectException<std::invalid_argument>([] { AcpHalAllocateShapeContexts(nullptr); });
        ExpectException<std::logic_error>([] { AcpHalReleaseShapeContexts(); });

        descriptor.counts = {4096, 4096, 4096, 4096, 512, 3584};
        Require(AcpHalAllocateShapeContexts(&descriptor) == 0, "Maximum SHAPE context configuration did not fit its own pool");
        constexpr std::array<std::uint32_t, 6> strides{32, 160, 32, 16, 64, 32};
        for (std::size_t index = 0; index < strides.size(); ++index) {
            auto* bytes = static_cast<std::byte*>(descriptor.contexts[index]);
            auto size = strides[index] * descriptor.counts[index];
            Require(bytes[0] == std::byte{0} && bytes[size - 1] == std::byte{0}, "Maximum SHAPE context was not initialized");
            Require(ApuMapApuAddress(descriptor.apuAddresses[index] + size - 1) == bytes + size - 1, "Maximum SHAPE context translation changed");
        }
        Require(AcpHalReleaseShapeContexts() == 0, "Maximum SHAPE context release failed");
        void* entireHeap = nullptr;
        Require(ApuAlloc(&entireHeap, nullptr, 0x228E00, 256, 2) == 0, "SHAPE release leaked storage from its own pool");
        ApuFree(entireHeap);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
