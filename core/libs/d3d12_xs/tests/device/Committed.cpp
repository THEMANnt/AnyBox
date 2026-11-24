#include "Tests.hpp"
#include "../../src/resource/Resource.hpp"
#include <anybox/vulkan/Image.hpp>
#include "TextureViews.hpp"
#include "RenderTargets.hpp"

void AnyBox::Graphics::Tests::CheckCommittedTextures(Device* device) {
    auto create = Method<decltype(&Device::CreateCommittedResource3)>(device, 127);
    D3D12_HEAP_PROPERTIES properties{};
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    properties.CreationNodeMask = 1;
    properties.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC1 description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    description.Width = 127;
    description.Height = 65;
    description.DepthOrArraySize = 3;
    description.MipLevels = 5;
    description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.SampleDesc.Count = 1;
    description.Flags = static_cast<D3D12_RESOURCE_FLAGS>(0x300005);
    D3D12_CLEAR_VALUE clear{};
    clear.Format = description.Format;
    void* output = nullptr;
    auto invoke = [&] { return create(device, &properties, D3D12_HEAP_FLAG_ALLOW_DISPLAY, &description, D3D12_BARRIER_LAYOUT_RENDER_TARGET, &clear, nullptr, 0, nullptr, ResourceId, &output); };
    Require(invoke() == S_OK && output, "Committed texture creation failed");
    auto release = [](Resource* value) { Method<decltype(&Resource::Release)>(value, 2)(value); };
    std::unique_ptr<Resource, decltype(release)> resource(static_cast<Resource*>(output), release);
    auto metadata = Method<decltype(&Resource::GetDescription1)>(resource.get(), 16)(resource.get());
    Require(metadata.Width == description.Width && metadata.MipLevels == 5 && metadata.DepthOrArraySize == 3 && metadata.Flags == description.Flags, "Committed texture lost its Xbox description");
    const auto& image = resource->GetGpuImage();
    Require(image.GetHandle() && image.GetDescription().mipLevels == 5 && image.GetDescription().arrayLayers == 3 && image.GetDescription().format == VK_FORMAT_R8G8B8A8_UNORM, "Committed texture has no matching GPU image");
    Require(Method<decltype(&Resource::GetGPUVirtualAddress)>(resource.get(), 11)(resource.get()) == 0, "Texture resource returned a buffer GPU address");
    auto heap = Method<decltype(&Resource::GetHeapProperties)>(resource.get(), 14);
    D3D12_HEAP_PROPERTIES returned{};
    D3D12_HEAP_FLAGS flags{};
    Require(heap(resource.get(), &returned, &flags) == S_OK && returned.Type == properties.Type && returned.VisibleNodeMask == 1 && flags == D3D12_HEAP_FLAG_ALLOW_DISPLAY, "Committed texture heap properties changed");
    Require(heap(resource.get(), nullptr, &flags) == S_OK, "Committed texture optional heap output failed");
    Reject<std::logic_error>([&] { resource->GetGpuBuffer(); });
    auto map = Method<decltype(&Resource::Map)>(resource.get(), 8);
    void* mapped = resource.get();
    Reject<std::runtime_error>([&] { map(resource.get(), 0, nullptr, &mapped); });
    Require(!mapped, "Rejected texture mapping retained a stale output");
    auto textureAddress = resource->GetTexture().GetAddress();
    MEMORY_BASIC_INFORMATION memory{};
    Require(VirtualQuery(reinterpret_cast<void*>(textureAddress), &memory, sizeof(memory)) == sizeof(memory) && memory.State == MEM_COMMIT && (memory.Protect & PAGE_WRITECOMBINE) != 0, "Committed texture did not use mapped write-combined XMem storage");
    CheckTextureShaderViews(device, resource.get());
    CheckTextureUnorderedViews(device, resource.get());
    CheckRenderTargets(device, resource.get());
    resource.reset();
    Require(VirtualQuery(reinterpret_cast<void*>(textureAddress), &memory, sizeof(memory)) == sizeof(memory) && memory.State == MEM_FREE, "Committed texture did not release its owned XMem storage");
    Reject<std::out_of_range>([&] { device->GetTextureAddresses().Resolve(textureAddress); });

    properties.Type = D3D12_HEAP_TYPE_UPLOAD;
    Reject<std::runtime_error>(invoke);
    Require(!output, "Rejected committed texture creation retained a stale output");
    properties.Type = D3D12_HEAP_TYPE_DEFAULT;
    description.SampleDesc.Count = 4;
    Reject<std::runtime_error>(invoke);
    description.SampleDesc.Count = 1;
    clear.Format = DXGI_FORMAT_R32_FLOAT;
    Reject<std::invalid_argument>(invoke);
    clear.Format = description.Format;
    Reject<std::runtime_error>([&] { create(device, &properties, D3D12_HEAP_FLAG_NONE, &description, D3D12_BARRIER_LAYOUT_COMMON, nullptr, resource.get(), 1, nullptr, ResourceId, &output); });
}
