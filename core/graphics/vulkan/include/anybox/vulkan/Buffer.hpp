#pragma once

#include "Context.hpp"
#include <cstddef>
#include <span>

namespace AnyBox::Vulkan {
class Buffer {
public:
    static Buffer Import(Context& context, std::span<std::byte> backing, VkDeviceSize size, VkBufferUsageFlags usage);
    static Buffer Allocate(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
    ~Buffer();
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) = delete;
    VkBuffer GetHandle() const noexcept { return buffer; }
    VkDeviceSize GetSize() const noexcept { return size; }
    VkDeviceAddress GetAddress() const;
    std::span<std::byte> Map();

private:
    Buffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage);
    void initialize(std::span<std::byte> backing, VkMemoryPropertyFlags properties);
    std::uint32_t chooseMemoryType(std::uint32_t bits, VkMemoryPropertyFlags required);
    Context& context;
    VkDeviceSize size;
    VkBufferUsageFlags usage;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryPropertyFlags properties = 0;
    std::byte* imported = nullptr;
    std::byte* mapped = nullptr;
};
}
