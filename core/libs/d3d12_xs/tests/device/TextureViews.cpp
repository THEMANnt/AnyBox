#include "Tests.hpp"
#include "TextureViews.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckTextureShaderViews(Device* device, Resource* resource) {
    auto create = Method<decltype(&Device::CreateShaderResourceView)>(device, 18);
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = resource->GetMetadata().Format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2DArray.MostDetailedMip = 1;
    view.Texture2DArray.MipLevels = 2;
    view.Texture2DArray.FirstArraySlice = 1;
    view.Texture2DArray.ArraySize = 1;
    alignas(16) std::array<std::uint32_t, 16> storage;
    storage.fill(UINT32_MAX);
    auto destination = reinterpret_cast<std::uintptr_t>(storage.data() + 4);
    create(device, resource, &view, destination);
    Descriptor::ImageWords words{};
    std::copy_n(storage.data() + 4, words.size(), words.data());
    auto address = (static_cast<std::uint64_t>(words[0]) << 8) | (static_cast<std::uint64_t>(words[1] & 255) << 40);
    auto& backing = device->GetTextureAddresses().Resolve(address);
    Require(&backing == &resource->GetTexture() && backing.GetView(words) != VK_NULL_HANDLE, "Texture SRV did not resolve to its actual Vulkan resource and image view");
    Require(((words[3] >> 12) & 15) == 1 && ((words[3] >> 16) & 15) == 2 && words[4] == 0x10001, "Texture SRV changed its mip or array range");
    Require(std::ranges::all_of(std::span(storage).first(4), [](auto value) { return value == UINT32_MAX; }) && std::ranges::all_of(std::span(storage).last(4), [](auto value) { return value == UINT32_MAX; }), "Texture SRV overwrote its neighboring descriptors");
    auto expected = storage;
    view.Texture2DArray.ArraySize = 4;
    Reject<std::out_of_range>([&] { create(device, resource, &view, destination); });
    view.Texture2DArray.ArraySize = 1;
    view.Texture2DArray.PlaneSlice = 1;
    Reject<std::runtime_error>([&] { create(device, resource, &view, destination); });
    view.Texture2DArray.PlaneSlice = 0;
    view.Texture2DArray.ResourceMinLODClamp = 2;
    Reject<std::runtime_error>([&] { create(device, resource, &view, destination); });
    view.Texture2DArray.ResourceMinLODClamp = 0;
    view.Format = DXGI_FORMAT_R32_UINT;
    Reject<std::runtime_error>([&] { create(device, resource, &view, destination); });
    view.Format = resource->GetMetadata().Format;
    Require(storage == expected, "Rejected texture SRV changed its destination");

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Texture SRV heap creation failed");
    auto release = [](DescriptorHeap* value) { Method<decltype(&DescriptorHeap::Release)>(value, 2)(value); };
    std::unique_ptr<DescriptorHeap, decltype(release)> heap(static_cast<DescriptorHeap*>(output), release);
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap.get(), 9)(heap.get());
    create(device, resource, &view, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(words))), "GPU did not observe the registered texture descriptor in write-combined memory");
    create(device, resource, nullptr, destination);
    std::copy_n(storage.data() + 4, words.size(), words.data());
    Require(backing.GetView(words) != VK_NULL_HANDLE && words[4] == 2 && ((words[3] >> 16) & 15) == 4, "Default texture SRV did not include its complete array and mip chain");
}
