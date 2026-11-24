#pragma once

#include "../device/Device.hpp"
#include "../host/Com.hpp"
#include <anybox/PhysicalMemory.hpp>
#include <anybox/vulkan/Buffer.hpp>

namespace AnyBox::Graphics {
struct BufferBacking {
    ComOwner<IPhysicalPageLease> pages;
    std::unique_ptr<Vulkan::Buffer> resource;
};
BufferBacking CreateBufferBacking(Device* device, std::uintptr_t address, const D3D12_RESOURCE_DESC1& description);
}
