#include "Tests.hpp"
#include "RenderTargets.hpp"
#include "../../src/descriptor/RenderTarget.hpp"
#include <algorithm>

void AnyBox::Graphics::Tests::CheckRenderTargets(Device* device, Resource* resource) {
    auto create = Method<decltype(&Device::CreateRenderTargetView)>(device, 20);
    alignas(16) std::array<std::byte, 144> storage;
    storage.fill(std::byte{0xA7});
    auto destination = reinterpret_cast<std::uintptr_t>(storage.data() + 16);
    auto checkGuards = [&] {
        Require(std::ranges::all_of(std::span(storage).first(16), [](auto value) { return value == std::byte{0xA7}; }) && std::ranges::all_of(std::span(storage).subspan(88), [](auto value) { return value == std::byte{0xA7}; }), "Render target view overwrote its prefix or unused heap stride bytes");
    };
    D3D12_RENDER_TARGET_VIEW_DESC description{};
    description.Format = resource->GetMetadata().Format;
    description.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
    description.Texture2DArray.MipSlice = 2;
    description.Texture2DArray.FirstArraySlice = 1;
    description.Texture2DArray.ArraySize = 1;
    create(device, resource, &description, destination);
    RenderTargetView view;
    std::memcpy(&view, storage.data() + 16, sizeof(view));
    Require(view.vtable && view.format == 28 && view.flags == 0 && view.resource == resource, "Render target view header does not match its resource");
    Require(view.registers[0] == 0x0010001F && view.registers[3] == 0x401F8040 && view.registers[4] == 0x08002001 && view.registers[11] == 2, "Render target view changed its mip dimensions or array range");
    auto address = (static_cast<std::uint64_t>(view.registers[1]) << 8) | (static_cast<std::uint64_t>(view.registers[2]) << 40);
    Require(&device->GetTextureAddresses().Resolve(address) == &resource->GetTexture(), "Render target GPU address resolved to another texture");
    auto* object = reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination));
    Require(Method<std::uint32_t (*)(void*)>(object, 0)(object) == 0, "Render target descriptor type differs from its native value");
    Reject<std::runtime_error>([&] { Method<void (*)(void*, ...)>(object, 1)(object); });
    checkGuards();

    auto expected = storage;
    description.Texture2DArray.MipSlice = 5;
    Reject<std::out_of_range>([&] { create(device, resource, &description, destination); });
    description.Texture2DArray.MipSlice = 2;
    description.Texture2DArray.PlaneSlice = 1;
    Reject<std::runtime_error>([&] { create(device, resource, &description, destination); });
    description.Texture2DArray.PlaneSlice = 0;
    description.Format = DXGI_FORMAT_R32_FLOAT;
    Reject<std::runtime_error>([&] { create(device, resource, &description, destination); });
    Require(storage == expected, "Rejected render target view changed its destination");
    create(device, resource, nullptr, destination);
    std::memcpy(&view, storage.data() + 16, sizeof(view));
    Require(view.registers[4] == 0x4000 && view.registers[11] == 0, "Default render target view lost its full resource array");
    checkGuards();
    create(device, nullptr, nullptr, destination);
    Require(std::ranges::all_of(std::span(storage).subspan(16, 72), [](auto value) { return value == std::byte{}; }), "Null render target view did not clear its 72-byte object");
    checkGuards();
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, destination + 1); });
}
