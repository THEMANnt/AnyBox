#include "Tests.hpp"
#include "../../src/descriptor/Descriptions.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckRawShaderViews(Device* device) {
    auto create = Method<decltype(&Device::CreateShaderResourceViewX)>(device, 70);
    D3D12_RESOURCE_DESC resource{};
    resource.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resource.Width = 1024;
    XboxShaderViewDescription description{};
    description.view.Format = DXGI_FORMAT_R32_TYPELESS;
    description.view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    description.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    description.view.Buffer.NumElements = 8;
    description.view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    description.address = 0x123456789000;
    alignas(16) std::array<std::uint32_t, 16> words{};
    auto destination = reinterpret_cast<std::uintptr_t>(words.data() + 4);
    for (auto first : {0ULL, 1ULL, 248ULL}) {
        description.view.Buffer.FirstElement = first;
        for (auto count : {0u, 8u, UINT32_MAX}) {
            description.view.Buffer.NumElements = count;
            words.fill(UINT32_MAX);
            create(device, &resource, &description, destination);
            auto address = description.address + first * 4;
            auto bytes = count == UINT32_MAX ? resource.Width - first * 4 : count * 4;
            Require(words[4] == static_cast<std::uint32_t>(address) && words[5] == address >> 32 && words[6] == bytes && words[7] == 0x30014FAC, "Xbox raw shader view address, byte extent or format encoding changed");
            Require(std::ranges::all_of(std::span(words).subspan(8, 4), [](auto value) { return value == 0; }), "Xbox raw shader view retained upper descriptor data");
            Require(std::ranges::all_of(std::span(words).first(4), [](auto value) { return value == UINT32_MAX; }) && std::ranges::all_of(std::span(words).subspan(12), [](auto value) { return value == UINT32_MAX; }), "Xbox raw shader view overwrote neighboring descriptors");
        }
    }
    description.view.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(4, 5, 0, 1);
    create(device, &resource, &description, destination);
    Require(words[7] == 0x30014B08, "Xbox raw shader view constant channel mapping changed");
    description.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    description.view.Buffer.FirstElement = 0;
    description.view.Buffer.NumElements = 8;
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, 0); });
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, destination + 1); });
    description.view.Buffer.NumElements = 257;
    Reject<std::out_of_range>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.NumElements = 8;
    description.parameters[0] = 1;
    Reject<std::runtime_error>([&] { create(device, &resource, &description, destination); });
    description.parameters[0] = 0;
    description.view.Shader4ComponentMapping = 6;
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, destination); });
    description.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    description.address += 1;
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, destination); });
    description.address -= 1;
    description.view.Buffer.FirstElement = UINT64_MAX;
    Reject<std::out_of_range>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.FirstElement = 0;
    description.view.Buffer.StructureByteStride = 4;
    Reject<std::invalid_argument>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.StructureByteStride = 0;
    description.view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    Reject<std::runtime_error>([&] { create(device, &resource, &description, destination); });
    description.view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    create(device, nullptr, &description, destination);
    Require(std::ranges::all_of(std::span(words).subspan(4, 8), [](auto value) { return value == 0; }), "Xbox extended null shader view did not clear its descriptor");

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Raw shader view heap creation failed");
    auto* heap = static_cast<DescriptorHeap*>(output);
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
    create(device, &resource, &description, pointer);
    create(device, &resource, &description, destination);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(words).subspan(4, 8))), "GPU did not observe the Xbox raw shader descriptor in write-combined memory");
    Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Raw shader view heap was not released");
}
