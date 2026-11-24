#include "Tests.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckNullShaderViews(Device* device) {
    auto create = Method<decltype(&Device::CreateShaderResourceView)>(device, 18);
    alignas(16) std::array<std::uint64_t, 4> words{};
    auto address = reinterpret_cast<std::uintptr_t>(words.data());
    D3D12_SHADER_RESOURCE_VIEW_DESC description{};
    description.Format = DXGI_FORMAT_R32_UINT;
    description.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    for (std::uint32_t dimension = 0; dimension <= 10; ++dimension) {
        words.fill(UINT64_MAX);
        description.ViewDimension = static_cast<D3D12_SRV_DIMENSION>(dimension);
        create(device, nullptr, &description, address);
        Require(std::ranges::all_of(words, [](auto value) { return value == 0; }), "Xbox null shader view did not clear all 32 bytes");
    }
    words.fill(UINT64_MAX);
    create(device, nullptr, nullptr, address);
    Require(std::ranges::all_of(words, [](auto value) { return value == 0; }), "Default Xbox null shader view did not clear all 32 bytes");
    description.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    description.RaytracingAccelerationStructure.Location = 0x123456789AB0;
    words.fill(UINT64_MAX);
    create(device, nullptr, &description, address);
    Require(words[0] == 0x123456789AB0 && words[1] == 0x1400000000000 && words[2] == UINT64_MAX && words[3] == UINT64_MAX, "Xbox acceleration structure descriptor layout changed");
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, 0); });
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, address + 1); });
    Reject<std::invalid_argument>([&] { create(device, reinterpret_cast<void*>(1), nullptr, address); });

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Null shader view heap creation failed");
    auto* heap = static_cast<DescriptorHeap*>(output);
    auto& buffer = heap->GetGpuBuffer();
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
    std::ranges::fill(buffer.Map(), std::byte{0xA7});
    create(device, nullptr, nullptr, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), buffer, scratch, 32);
    Require(std::ranges::all_of(scratch.Map(), [](auto value) { return value == std::byte{}; }), "GPU did not observe the Xbox null shader view in write-combined memory");
    Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Null shader view heap was not released");
}
