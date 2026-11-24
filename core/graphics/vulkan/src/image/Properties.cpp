#include <anybox/vulkan/Context.hpp>

VkImageFormatProperties AnyBox::Vulkan::Context::GetImageFormatProperties(const VkPhysicalDeviceImageFormatInfo2& description) const {
    VkImageFormatProperties2 output{};
    output.sType = VK_STRUCTURE_TYPE_IMAGE_FORMAT_PROPERTIES_2;
    Check(instanceFunctions.getImageFormatProperties(physicalDevice, &description, &output), "Query Vulkan image format properties");
    return output.imageFormatProperties;
}
