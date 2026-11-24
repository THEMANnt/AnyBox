#include "Tests.hpp"
#include "TextureViews.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckTextureUnorderedViews(Device* device, Resource* resource) {
    auto create = Method<decltype(&Device::CreateUnorderedAccessView)>(device, 19);
    D3D12_UNORDERED_ACCESS_VIEW_DESC view{};
    view.Format = resource->GetMetadata().Format;
    view.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
    view.Texture2DArray.MipSlice = 2;
    view.Texture2DArray.FirstArraySlice = 1;
    view.Texture2DArray.ArraySize = 2;
    alignas(16) std::array<std::uint32_t, 16> storage;
    storage.fill(UINT32_MAX);
    auto destination = reinterpret_cast<std::uintptr_t>(storage.data() + 4);
    create(device, resource, nullptr, &view, destination);
    Descriptor::ImageWords words{};
    std::copy_n(storage.data() + 4, words.size(), words.data());
    auto address = (static_cast<std::uint64_t>(words[0]) << 8) | (static_cast<std::uint64_t>(words[1] & 255) << 40);
    auto& backing = device->GetTextureAddresses().Resolve(address);
    Require(&backing == &resource->GetTexture() && backing.GetStorageView(words), "Texture UAV did not resolve to its actual GPU storage view");
    Require(((words[3] >> 12) & 15) == 2 && ((words[3] >> 16) & 15) == 2 && words[4] == 0x10002 && words[5] == 0x80000040, "Texture UAV changed its single mip, array range or storage usage");
    Require(std::ranges::all_of(std::span(storage).first(4), [](auto value) { return value == UINT32_MAX; }) && std::ranges::all_of(std::span(storage).last(4), [](auto value) { return value == UINT32_MAX; }), "Texture UAV overwrote neighboring descriptors");
    auto expected = storage;
    Reject<std::invalid_argument>([&] { create(device, resource, resource, &view, destination); });
    view.Texture2DArray.MipSlice = 5;
    Reject<std::out_of_range>([&] { create(device, resource, nullptr, &view, destination); });
    view.Texture2DArray.MipSlice = 2;
    view.Texture2DArray.PlaneSlice = 1;
    Reject<std::runtime_error>([&] { create(device, resource, nullptr, &view, destination); });
    view.Texture2DArray.PlaneSlice = 0;
    view.Format = DXGI_FORMAT_R32_UINT;
    Reject<std::runtime_error>([&] { create(device, resource, nullptr, &view, destination); });
    view.Format = resource->GetMetadata().Format;
    Require(storage == expected, "Rejected texture UAV changed its destination");

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Texture UAV heap creation failed");
    auto release = [](DescriptorHeap* value) { Method<decltype(&DescriptorHeap::Release)>(value, 2)(value); };
    std::unique_ptr<DescriptorHeap, decltype(release)> heap(static_cast<DescriptorHeap*>(output), release);
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap.get(), 9)(heap.get());
    create(device, resource, nullptr, &view, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(words))), "GPU did not observe the Xbox texture UAV in write-combined memory");
    create(device, resource, nullptr, nullptr, destination);
    std::copy_n(storage.data() + 4, words.size(), words.data());
    Require(words[4] == 2 && ((words[3] >> 12) & 15) == 0 && ((words[3] >> 16) & 15) == 0 && backing.GetStorageView(words), "Default texture UAV lost its base mip or complete array");
}
