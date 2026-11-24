#include <anybox/vulkan/ImageView.hpp>
#include <utility>

namespace {
VkImageAspectFlags FormatAspects(VkFormat format) {
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT: return VK_IMAGE_ASPECT_DEPTH_BIT;
        case VK_FORMAT_S8_UINT: return VK_IMAGE_ASPECT_STENCIL_BIT;
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT: return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        default: return VK_IMAGE_ASPECT_COLOR_BIT;
    }
}

void CheckType(const AnyBox::Vulkan::ImageDescription& image, const AnyBox::Vulkan::ImageViewDescription& view) {
    switch (view.type) {
        case VK_IMAGE_VIEW_TYPE_1D:
        case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
            if (image.type != VK_IMAGE_TYPE_1D) throw std::invalid_argument("Vulkan 1D view requires a 1D image");
            break;
        case VK_IMAGE_VIEW_TYPE_2D:
        case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
            if (image.type != VK_IMAGE_TYPE_2D) throw std::invalid_argument("Vulkan 2D view requires a 2D image");
            break;
        case VK_IMAGE_VIEW_TYPE_3D:
            if (image.type != VK_IMAGE_TYPE_3D) throw std::invalid_argument("Vulkan 3D view requires a 3D image");
            break;
        case VK_IMAGE_VIEW_TYPE_CUBE:
        case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
            if ((image.flags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) == 0 || view.layerCount % 6 != 0 || (view.type == VK_IMAGE_VIEW_TYPE_CUBE && view.layerCount != 6)) throw std::invalid_argument("Vulkan cube view requires compatible image faces");
            break;
        default: throw std::runtime_error("Vulkan image view type is not implemented");
    }
    if ((view.type == VK_IMAGE_VIEW_TYPE_1D || view.type == VK_IMAGE_VIEW_TYPE_2D || view.type == VK_IMAGE_VIEW_TYPE_3D) && view.layerCount != 1) throw std::invalid_argument("Non-array Vulkan image view has multiple layers");
}
}

AnyBox::Vulkan::ImageView::ImageView(Image& image, const ImageViewDescription& value) : context(image.GetContext()), description(value) {
    const auto& source = image.GetDescription();
    if (!image.GetHandle()) throw std::invalid_argument("Vulkan image view source has no allocation");
    if (description.firstMip >= source.mipLevels || description.firstLayer >= source.arrayLayers) throw std::out_of_range("Vulkan image view starts outside its image");
    if (description.mipCount == VK_REMAINING_MIP_LEVELS) description.mipCount = source.mipLevels - description.firstMip;
    if (description.layerCount == VK_REMAINING_ARRAY_LAYERS) description.layerCount = source.arrayLayers - description.firstLayer;
    if (description.mipCount == 0 || description.mipCount > source.mipLevels - description.firstMip || description.layerCount == 0 || description.layerCount > source.arrayLayers - description.firstLayer) throw std::out_of_range("Vulkan image view exceeds its image");
    constexpr auto usages = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    if ((source.usage & usages) == 0) throw std::invalid_argument("Vulkan image was not created for views");
    if (description.aspects == 0 || (description.aspects & ~FormatAspects(source.format)) != 0) throw std::invalid_argument("Vulkan image view aspects do not match its format");
    for (auto component : {description.components.r, description.components.g, description.components.b, description.components.a}) {
        if (component < VK_COMPONENT_SWIZZLE_IDENTITY || component > VK_COMPONENT_SWIZZLE_A) throw std::invalid_argument("Vulkan image view component swizzle is invalid");
    }
    if ((description.aspects & VK_IMAGE_ASPECT_COLOR_BIT) == 0 && (description.components.r != VK_COMPONENT_SWIZZLE_IDENTITY || description.components.g != VK_COMPONENT_SWIZZLE_IDENTITY || description.components.b != VK_COMPONENT_SWIZZLE_IDENTITY || description.components.a != VK_COMPONENT_SWIZZLE_IDENTITY)) throw std::invalid_argument("Depth or stencil Vulkan views cannot swizzle components");
    CheckType(source, description);
    if (description.type == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY && !context.SupportsCubeArrays()) throw std::runtime_error("Vulkan cube image arrays are unsupported");

    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image.GetHandle();
    info.viewType = description.type;
    info.format = source.format;
    info.components = description.components;
    info.subresourceRange = {description.aspects, description.firstMip, description.mipCount, description.firstLayer, description.layerCount};
    Check(context.GetFunctions().createImageView(context.GetDevice(), &info, nullptr, &view), "Create Vulkan image view");
}

AnyBox::Vulkan::ImageView::ImageView(ImageView&& other) noexcept : context(other.context), description(other.description), view(std::exchange(other.view, VK_NULL_HANDLE)) {}

AnyBox::Vulkan::ImageView::~ImageView() {
    if (view) context.GetFunctions().destroyImageView(context.GetDevice(), view, nullptr);
}
