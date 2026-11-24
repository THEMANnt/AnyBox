#pragma once

#include "../device/Device.hpp"
#include <anybox/vulkan/Buffer.hpp>
#include <memory>

namespace AnyBox::Graphics {
struct DescriptorBacking {
    DescriptorBacking() = default;
    DescriptorBacking(DescriptorBacking&& other) noexcept;
    DescriptorBacking(const DescriptorBacking&) = delete;
    DescriptorBacking& operator=(const DescriptorBacking&) = delete;
    ~DescriptorBacking();
    void* address = nullptr;
    std::size_t size = 0;
    std::uint64_t attributes = 0;
    std::unique_ptr<Vulkan::Buffer> buffer;
};
DescriptorBacking CreateDescriptorBacking(Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description);
}
