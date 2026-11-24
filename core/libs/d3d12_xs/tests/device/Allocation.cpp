#include "Tests.hpp"
#include <array>
#include <limits>

void AnyBox::Graphics::Tests::CheckAllocation(Device* device) {
    auto query = Method<decltype(&Device::GetResourceAllocationInfo)>(device, 25);
    auto empty = query(device, 0, 0, nullptr);
    Require(empty.SizeInBytes == 0 && empty.Alignment == 1, "Empty resource allocation query changed its bounds");
    Reject<std::invalid_argument>([&] { query(device, 0, 1, nullptr); });
    std::array<D3D12_RESOURCE_DESC, 3> descriptions{};
    for (auto& description : descriptions) description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    descriptions[0].Width = 3;
    descriptions[1].Width = 5;
    descriptions[1].Alignment = 16;
    descriptions[2].Width = 7;
    auto buffers = query(device, 0, 3, descriptions.data());
    Require(buffers.SizeInBytes == 31 && buffers.Alignment == 16, "Buffer allocation array has incorrect padding");
    descriptions[0].Width = std::numeric_limits<std::uint64_t>::max();
    Reject<std::overflow_error>([&] { query(device, 0, 3, descriptions.data()); });
    descriptions[0].Width = 1;
    descriptions[1].Alignment = 17;
    Reject<std::invalid_argument>([&] { query(device, 0, 3, descriptions.data()); });

    D3D12_RESOURCE_DESC texture{};
    texture.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texture.Width = 63;
    texture.Height = 17;
    texture.DepthOrArraySize = 1;
    texture.MipLevels = 1;
    texture.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texture.SampleDesc.Count = 1;
    auto check = [&](std::uint32_t flags, std::uint64_t size, std::uint64_t alignment) {
        texture.Flags = static_cast<D3D12_RESOURCE_FLAGS>(flags);
        auto result = query(device, 0, 1, &texture);
        Require(result.SizeInBytes == size && result.Alignment == alignment, "Texture allocation differs from its reference geometry");
    };
    check(0, 8192, 4096);
    check(4, 8192, 4096);
    check(0x300005, 65536, 65536);
    texture.Width = 1024;
    texture.Height = 1024;
    texture.MipLevels = 11;
    check(0x300005, 5636096, 65536);
    texture.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    Reject<std::runtime_error>([&] { query(device, 0, 1, &texture); });
    texture.Flags = D3D12_RESOURCE_FLAG_NONE;
    texture.SampleDesc.Count = 4;
    Reject<std::runtime_error>([&] { query(device, 0, 1, &texture); });

    constexpr std::array<std::array<std::uint32_t, 6>, 8> references{{
        {1024, 1024, 11, 1, 2, 22413312},
        {1024, 1024, 11, 1, 10, 11206656},
        {1024, 1024, 11, 1, 49, 2818048},
        {1024, 1024, 11, 1, 61, 1441792},
        {256, 256, 9, 6, 2, 8650752},
        {256, 256, 9, 6, 10, 4325376},
        {256, 256, 9, 6, 49, 1179648},
        {256, 256, 9, 6, 61, 786432}
    }};
    texture.SampleDesc.Count = 1;
    for (const auto& reference : references) {
        texture.Width = reference[0];
        texture.Height = reference[1];
        texture.MipLevels = static_cast<std::uint16_t>(reference[2]);
        texture.DepthOrArraySize = static_cast<std::uint16_t>(reference[3]);
        texture.Format = static_cast<DXGI_FORMAT>(reference[4]);
        check(0x300005, reference[5], 65536);
    }
}
