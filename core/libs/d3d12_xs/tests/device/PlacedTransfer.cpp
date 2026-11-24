#include "Tests.hpp"
#include "PlacedTransfer.hpp"
#include <anybox/vulkan/MemoryTest.hpp>
#include <algorithm>

void AnyBox::Graphics::Tests::CheckPlacedBufferTransfer(Device* device, Resource* resource, std::span<std::byte> memory, bool repeated) {
    auto& context = device->GetGpuContext();
    auto& buffer = resource->GetGpuBuffer();
    Require(buffer.GetSize() == memory.size() && buffer.Map().data() == memory.data(), "Xbox placed buffer lost its mapped physical memory");
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratch = Vulkan::Buffer::Allocate(context, memory.size(), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    for (std::size_t index = 0; index < memory.size(); ++index) memory[index] = static_cast<std::byte>((index * 71) ^ (index >> 9));
    std::vector<std::byte> expected(memory.begin(), memory.end());
    Vulkan::Tests::Copy(context, buffer, scratch, memory.size());
    auto observed = scratch.Map();
    Require(std::ranges::equal(expected, observed), "Vulkan did not read the Xbox resource's physical pages");

    for (std::size_t index = 0; index < observed.size(); ++index) observed[index] = static_cast<std::byte>((index * 37) ^ (index >> 8));
    Vulkan::Tests::Copy(context, scratch, buffer, repeated ? PhysicalPageSize : memory.size());
    Require(std::ranges::equal(observed, memory), "Vulkan did not write the Xbox resource's physical pages");
    Vulkan::Tests::CheckCompute(context, buffer, scratch, repeated);
}
