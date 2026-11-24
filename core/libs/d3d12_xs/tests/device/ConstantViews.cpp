#include "Tests.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckConstantViews(Device* device) {
    auto create = Method<decltype(&Device::CreateConstantBufferView)>(device, 17);
    alignas(16) std::array<std::uint64_t, 8> words{};
    auto destination = reinterpret_cast<std::uintptr_t>(words.data() + 2);
    for (auto address : {0ULL, 0x123456789AB0ULL, UINT64_MAX}) {
        for (auto size : {0u, 1u, 15u, 16u, 17u, 255u, 256u, 257u, UINT32_MAX}) {
            D3D12_CONSTANT_BUFFER_VIEW_DESC description{address, size};
            words.fill(UINT64_MAX);
            create(device, &description, destination);
            auto count = (static_cast<std::uint64_t>(size) + 15) >> 4;
            Require(words[2] == address + 0x10000000000000ULL && words[3] == count + 0x2201403C00000000ULL, "Xbox constant view address or 16-byte count encoding changed");
            Require(words[0] == UINT64_MAX && words[1] == UINT64_MAX && std::ranges::all_of(std::span(words).subspan(4), [](auto value) { return value == UINT64_MAX; }), "Xbox constant view overwrote bytes outside its 16-byte descriptor");
        }
    }
    words.fill(UINT64_MAX);
    create(device, nullptr, destination);
    auto address = words[2] - 0x10000000000000ULL;
    Require(address != 0 && address <= 0xFFFFFFFFFFFFULL && address % 16 == 0 && words[3] == 0x2201403C00000001ULL, "Xbox null constant view did not encode its real zero buffer");
    std::array<std::byte, 16> zero{};
    Vulkan::Tests::CheckAddress(device->GetGpuContext(), address, zero);
    create(device, nullptr, destination);
    Require(words[2] == address + 0x10000000000000ULL, "Xbox null constant view changed its buffer address between calls");
    Reject<std::invalid_argument>([&] { create(device, nullptr, 0); });
    Reject<std::invalid_argument>([&] { create(device, nullptr, destination + 1); });
}
