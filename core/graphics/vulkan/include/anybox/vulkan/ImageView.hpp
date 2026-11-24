#pragma once

#include "Image.hpp"

namespace AnyBox::Vulkan {
struct ImageViewDescription {
    VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
    VkImageAspectFlags aspects = VK_IMAGE_ASPECT_COLOR_BIT;
    VkComponentMapping components{};
    std::uint32_t firstMip = 0;
    std::uint32_t mipCount = VK_REMAINING_MIP_LEVELS;
    std::uint32_t firstLayer = 0;
    std::uint32_t layerCount = VK_REMAINING_ARRAY_LAYERS;
};

class ImageView {
public:
    ImageView(Image& image, const ImageViewDescription& description);
    ~ImageView();
    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&& other) noexcept;
    ImageView& operator=(ImageView&& other) = delete;
    VkImageView GetHandle() const noexcept { return view; }
    const ImageViewDescription& GetDescription() const noexcept { return description; }

private:
    Context& context;
    ImageViewDescription description;
    VkImageView view = VK_NULL_HANDLE;
};
}
