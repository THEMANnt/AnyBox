#pragma once

#include "Context.hpp"

namespace AnyBox::Vulkan {
struct ImageDescription {
    VkImageType type = VK_IMAGE_TYPE_2D;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent{};
    std::uint32_t mipLevels = 1;
    std::uint32_t arrayLayers = 1;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageUsageFlags usage = 0;
    VkImageCreateFlags flags = 0;
};

class Image {
public:
    Image(Context& context, const ImageDescription& description);
    ~Image();
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) = delete;
    VkImage GetHandle() const noexcept { return image; }
    VkDeviceSize GetAllocationSize() const noexcept { return allocationSize; }
    const ImageDescription& GetDescription() const noexcept { return description; }
    Context& GetContext() const noexcept { return context; }

private:
    void create();
    void allocate();
    void release() noexcept;
    Context& context;
    ImageDescription description;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize allocationSize = 0;
};
}
