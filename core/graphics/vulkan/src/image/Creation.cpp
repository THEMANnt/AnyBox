#include <anybox/vulkan/Image.hpp>
#include <algorithm>
#include <bit>

void AnyBox::Vulkan::Image::create() {
    const auto& value = description;
    auto samples = static_cast<std::uint32_t>(value.samples);
    if (value.type < VK_IMAGE_TYPE_1D || value.type > VK_IMAGE_TYPE_3D || value.extent.width == 0 || value.extent.height == 0 || value.extent.depth == 0 || value.arrayLayers == 0) throw std::invalid_argument("Vulkan image type, extent or layer count is invalid");
    auto maximumDimension = std::max({value.extent.width, value.extent.height, value.extent.depth});
    if (value.mipLevels == 0 || value.mipLevels > static_cast<std::uint32_t>(std::bit_width(maximumDimension)) || !std::has_single_bit(samples) || samples > 64 || (samples != 1 && value.mipLevels != 1)) throw std::invalid_argument("Vulkan image mip or sample count is invalid");
    if ((value.type == VK_IMAGE_TYPE_1D && value.extent.height != 1) || (value.type != VK_IMAGE_TYPE_3D && value.extent.depth != 1) || (value.type == VK_IMAGE_TYPE_3D && value.arrayLayers != 1)) throw std::invalid_argument("Vulkan image dimensions do not match its type");
    if ((value.flags & ~VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0) throw std::runtime_error("Vulkan image creation flags are not implemented");
    if ((value.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) != 0 && (value.type != VK_IMAGE_TYPE_2D || value.extent.width != value.extent.height || value.arrayLayers < 6 || samples != 1)) throw std::invalid_argument("Vulkan cube image dimensions or sample count are invalid");
    if (value.format <= VK_FORMAT_UNDEFINED || value.format > VK_FORMAT_ASTC_12x12_SRGB_BLOCK) throw std::runtime_error("Vulkan image format is not implemented");
    if (value.usage == 0 || (value.usage & ~0xFFU) != 0) throw std::runtime_error("Vulkan image usage is not implemented");
    constexpr auto attachments = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if ((value.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) != 0 && (value.usage & ~attachments) != 0) throw std::invalid_argument("Transient Vulkan image has non-attachment usage");

    VkPhysicalDeviceImageFormatInfo2 query{};
    query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_IMAGE_FORMAT_INFO_2;
    query.format = value.format;
    query.type = value.type;
    query.tiling = VK_IMAGE_TILING_OPTIMAL;
    query.usage = value.usage;
    query.flags = value.flags;
    auto supported = context.GetImageFormatProperties(query);
    if (value.extent.width > supported.maxExtent.width || value.extent.height > supported.maxExtent.height || value.extent.depth > supported.maxExtent.depth || value.mipLevels > supported.maxMipLevels || value.arrayLayers > supported.maxArrayLayers || (supported.sampleCounts & value.samples) == 0) throw std::runtime_error("Vulkan image dimensions or samples exceed format limits");

    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.flags = value.flags;
    info.imageType = value.type;
    info.format = value.format;
    info.extent = value.extent;
    info.mipLevels = value.mipLevels;
    info.arrayLayers = value.arrayLayers;
    info.samples = value.samples;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = value.usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    Check(context.GetFunctions().createImage(context.GetDevice(), &info, nullptr, &image), "Create Vulkan image");
}
