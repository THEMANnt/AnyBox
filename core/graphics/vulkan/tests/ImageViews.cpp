#include "Tests.hpp"
#include <anybox/vulkan/ImageView.hpp>
#include <utility>

namespace {
template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    bool rejected = false;
    try { callable(); } catch (const TException&) { rejected = true; }
    AnyBox::Vulkan::Tests::Require(rejected, "Invalid Vulkan image view was accepted");
}
}

void AnyBox::Vulkan::Tests::CheckImageViews(Context& context) {
    ImageDescription description;
    description.format = VK_FORMAT_R8G8B8A8_UNORM;
    description.extent = {32, 32, 1};
    description.mipLevels = 6;
    description.arrayLayers = 12;
    description.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    description.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    Image image(context, description);
    ImageViewDescription view;
    view.type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    view.firstMip = 2;
    view.firstLayer = 3;
    ImageView array(image, view);
    Require(array.GetHandle() && array.GetDescription().mipCount == 4 && array.GetDescription().layerCount == 9, "Vulkan array view lost its remaining subresource range");
    ImageView moved(std::move(array));
    Require(moved.GetHandle() && !array.GetHandle(), "Moved Vulkan image view ownership was duplicated");
    view.type = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
    view.firstLayer = 0;
    ImageView cubes(image, view);
    view.type = VK_IMAGE_VIEW_TYPE_CUBE;
    view.layerCount = 6;
    ImageView cube(image, view);
    view.type = VK_IMAGE_VIEW_TYPE_2D;
    view.layerCount = 1;
    view.components = {VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_ONE};
    ImageView swizzled(image, view);
    Require(cubes.GetHandle() && cube.GetHandle() && swizzled.GetHandle(), "Vulkan cube or swizzled image view creation failed");

    view.firstMip = 6;
    Reject<std::out_of_range>([&] { ImageView invalid(image, view); });
    view.firstMip = 0;
    view.mipCount = 0;
    Reject<std::out_of_range>([&] { ImageView invalid(image, view); });
    view.mipCount = 1;
    view.layerCount = 2;
    Reject<std::invalid_argument>([&] { ImageView invalid(image, view); });
    view.layerCount = 1;
    view.aspects = VK_IMAGE_ASPECT_DEPTH_BIT;
    Reject<std::invalid_argument>([&] { ImageView invalid(image, view); });
    view.aspects = VK_IMAGE_ASPECT_COLOR_BIT;
    view.type = VK_IMAGE_VIEW_TYPE_3D;
    Reject<std::invalid_argument>([&] { ImageView invalid(image, view); });
    view.type = VK_IMAGE_VIEW_TYPE_CUBE;
    view.layerCount = 5;
    Reject<std::invalid_argument>([&] { ImageView invalid(image, view); });
}
