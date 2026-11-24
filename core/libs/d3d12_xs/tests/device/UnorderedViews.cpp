#include "Tests.hpp"
#include "../../src/descriptor/Heap.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckNullUnorderedViews(Device* device) {
    auto create = Method<decltype(&Device::CreateUnorderedAccessView)>(device, 19);
    alignas(16) std::array<std::uint64_t, 8> words{};
    auto address = reinterpret_cast<std::uintptr_t>(words.data() + 2);
    D3D12_UNORDERED_ACCESS_VIEW_DESC description{};
    description.Format = DXGI_FORMAT_R32_UINT;
    for (std::uint32_t dimension = 0; dimension <= 11; ++dimension) {
        words.fill(UINT64_MAX);
        description.ViewDimension = static_cast<D3D12_UAV_DIMENSION>(dimension);
        create(device, nullptr, nullptr, &description, address);
        Require(std::ranges::all_of(std::span(words).subspan(2, 4), [](auto value) { return value == 0; }), "Xbox null unordered view did not clear all 32 bytes");
        Require(words[0] == UINT64_MAX && words[1] == UINT64_MAX && words[6] == UINT64_MAX && words[7] == UINT64_MAX, "Xbox null unordered view overwrote its neighboring descriptors");
    }
    words.fill(UINT64_MAX);
    create(device, nullptr, reinterpret_cast<void*>(1), nullptr, address);
    Require(words[2] == 0 && words[3] == 0 && words[4] == 0 && words[5] == 0, "Xbox null resource view did not ignore its unused counter resource");
    description.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    description.Buffer.FirstElement = 0xF123456789ABCDEFULL;
    for (auto format : {DXGI_FORMAT_UNKNOWN, DXGI_FORMAT_R32_UINT}) {
        description.Format = format;
        for (auto flags : {0u, 1u, 2u, 3u}) {
            description.Buffer.Flags = static_cast<D3D12_BUFFER_UAV_FLAGS>(flags);
            for (auto stride : {0u, 4u, 0x4001u, UINT32_MAX}) {
                description.Buffer.StructureByteStride = stride;
                words.fill(UINT64_MAX);
                create(device, nullptr, nullptr, &description, address);
                auto expected = format != DXGI_FORMAT_UNKNOWN && flags == 2 && stride != 0 ? std::uint64_t{0x89ABCDEF} | (static_cast<std::uint64_t>(stride & 0x3FFF) << 48) : 0;
                Require(words[2] == expected && words[3] == 0 && words[4] == 0 && words[5] == 0, "Xbox null strided unordered view encoding changed");
            }
        }
    }
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, nullptr, 0); });
    Reject<std::invalid_argument>([&] { create(device, nullptr, nullptr, nullptr, address + 1); });
    Reject<std::invalid_argument>([&] { create(device, reinterpret_cast<void*>(1), nullptr, nullptr, address); });

    D3D12_DESCRIPTOR_HEAP_DESC heapDescription{D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateDescriptorHeap)>(device, 14)(device, &heapDescription, DescriptorHeapId, &output) == S_OK && output, "Null unordered view heap creation failed");
    auto* heap = static_cast<DescriptorHeap*>(output);
    auto& buffer = heap->GetGpuBuffer();
    auto pointer = Method<decltype(&DescriptorHeap::GetCpuStart)>(heap, 9)(heap);
    std::ranges::fill(buffer.Map(), std::byte{0xA7});
    create(device, nullptr, nullptr, nullptr, pointer);
    auto scratch = Vulkan::Buffer::Allocate(device->GetGpuContext(), 32, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Vulkan::Tests::Copy(device->GetGpuContext(), buffer, scratch, 32);
    Require(std::ranges::all_of(scratch.Map(), [](auto value) { return value == std::byte{}; }), "GPU did not observe the Xbox null unordered view in write-combined memory");
    Require(Method<decltype(&DescriptorHeap::Release)>(heap, 2)(heap) == 0, "Null unordered view heap was not released");
}
