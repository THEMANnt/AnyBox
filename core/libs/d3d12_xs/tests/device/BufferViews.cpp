#include "Tests.hpp"
#include "BufferViews.hpp"
#include "../../src/descriptor/Descriptions.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckBufferViews(Device* device, Resource* resource) {
    auto create = Method<decltype(&Device::CreateUnorderedAccessView)>(device, 19);
    auto width = resource->GetMetadata().Width;
    auto base = Method<decltype(&Resource::GetGPUVirtualAddress)>(resource, 11)(resource);
    D3D12_UNORDERED_ACCESS_VIEW_DESC description{};
    description.Format = DXGI_FORMAT_R32_TYPELESS;
    description.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    description.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
    alignas(16) std::array<std::uint32_t, 16> words{};
    auto destination = reinterpret_cast<std::uintptr_t>(words.data() + 4);
    for (auto first : std::array<std::uint64_t, 3>{0, 1, width / 4 - 8}) {
        description.Buffer.FirstElement = first;
        for (auto count : {0u, 8u, UINT32_MAX}) {
            description.Buffer.NumElements = count;
            words.fill(UINT32_MAX);
            create(device, resource, nullptr, &description, destination);
            auto address = base + first * 4;
            auto bytes = count == UINT32_MAX ? width - first * 4 : count * 4;
            Require(words[4] == static_cast<std::uint32_t>(address) && words[5] == address >> 32 && words[6] == bytes && words[7] == 0x30014204, "Xbox raw unordered view address, byte extent or channel mapping changed");
            Require(std::ranges::all_of(std::span(words).subspan(8, 4), [](auto value) { return value == 0; }), "Xbox raw unordered view did not clear its unused counter address");
            Require(std::ranges::all_of(std::span(words).first(4), [](auto value) { return value == UINT32_MAX; }) && std::ranges::all_of(std::span(words).subspan(12), [](auto value) { return value == UINT32_MAX; }), "Xbox raw unordered view overwrote neighboring descriptors");
        }
    }
    description.Buffer.FirstElement = 0;
    description.Buffer.NumElements = 8;
    create(device, resource, nullptr, &description, destination);
    auto encoded = words;
    Reject<std::invalid_argument>([&] { create(device, resource, resource, &description, destination); });
    description.Buffer.CounterOffsetInBytes = 4;
    Reject<std::invalid_argument>([&] { create(device, resource, nullptr, &description, destination); });
    description.Buffer.CounterOffsetInBytes = 0;
    description.Buffer.StructureByteStride = 4;
    Reject<std::invalid_argument>([&] { create(device, resource, nullptr, &description, destination); });
    description.Buffer.StructureByteStride = 0;
    description.Buffer.FirstElement = UINT64_MAX;
    Reject<std::out_of_range>([&] { create(device, resource, nullptr, &description, destination); });
    description.Buffer.FirstElement = 0;
    Reject<std::runtime_error>([&] { create(device, resource, nullptr, nullptr, destination); });
    Reject<std::invalid_argument>([&] { create(device, reinterpret_cast<void*>(1), nullptr, &description, destination); });
    alignas(Resource) std::array<std::byte, sizeof(void*)> foreign{};
    Reject<std::invalid_argument>([&] { create(device, foreign.data(), nullptr, &description, destination); });
    Reject<std::invalid_argument>([&] { create(nullptr, resource, nullptr, &description, destination); });
    Require(words == encoded, "Rejected Xbox raw unordered view changed its destination");

    auto metadata = Method<decltype(&Resource::GetDescription)>(resource, 10)(resource);
    XboxShaderViewDescription shader{};
    shader.view.Format = DXGI_FORMAT_R32_TYPELESS;
    shader.view.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    shader.view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    shader.view.Buffer.NumElements = 8;
    shader.view.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
    shader.address = base;
    Method<decltype(&Device::CreateShaderResourceViewX)>(device, 70)(device, &metadata, &shader, destination);
    Require(words[4] == encoded[4] && words[5] == encoded[5] && words[6] == encoded[6] && words[7] == 0x30014FAC, "Xbox raw SRV and UAV did not preserve their distinct channel mappings");
    auto shaderEncoded = words;
    auto shaderCreate = Method<decltype(&Device::CreateShaderResourceView)>(device, 18);
    words.fill(UINT32_MAX);
    shaderCreate(device, resource, &shader.view, destination);
    Require(words == shaderEncoded, "Resource shader view differs from its explicit address encoding");
    shader.view.Buffer.StructureByteStride = 4;
    Reject<std::invalid_argument>([&] { shaderCreate(device, resource, &shader.view, destination); });
    shader.view.Buffer.StructureByteStride = 0;
    shader.view.Buffer.FirstElement = width / 4;
    Reject<std::out_of_range>([&] { shaderCreate(device, resource, &shader.view, destination); });
    shader.view.Buffer.FirstElement = 0;
    Reject<std::invalid_argument>([&] { shaderCreate(nullptr, resource, &shader.view, destination); });
    Require(words == shaderEncoded, "Rejected shader view changed its destination");

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Raw unordered view heap creation failed");
    auto* heap = static_cast<DescriptorHeap*>(output);
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
    create(device, resource, nullptr, &description, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(encoded).subspan(4, 8))), "GPU did not observe the Xbox raw unordered descriptor in write-combined memory");
    shaderCreate(device, resource, &shader.view, pointer);
    Vulkan::Tests::Copy(device->GetGpuContext(), heap->GetGpuBuffer(), scratch, 32);
    Require(std::ranges::equal(scratch.Map(), std::as_bytes(std::span(shaderEncoded).subspan(4, 8))), "GPU did not observe the Xbox raw shader descriptor in write-combined memory");
    Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Raw unordered view heap was not released");
}
