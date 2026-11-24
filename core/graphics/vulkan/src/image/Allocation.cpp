#include <anybox/vulkan/Image.hpp>
#include <limits>

void AnyBox::Vulkan::Image::allocate() {
    const auto& functions = context.GetFunctions();
    VkImageMemoryRequirementsInfo2 info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2;
    info.image = image;
    VkMemoryDedicatedRequirements dedicatedRequirements{};
    dedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
    VkMemoryRequirements2 requirements{};
    requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    requirements.pNext = &dedicatedRequirements;
    functions.getImageRequirements(context.GetDevice(), &info, &requirements);
    auto type = std::numeric_limits<std::uint32_t>::max();
    const auto& properties = context.GetMemoryProperties();
    for (std::uint32_t index = 0; index < properties.memoryTypeCount; ++index) {
        if ((requirements.memoryRequirements.memoryTypeBits & (1U << index)) != 0 && (properties.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            type = index;
            break;
        }
    }
    if (type == std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("Vulkan image has no compatible device-local memory type");

    VkMemoryDedicatedAllocateInfo dedicated{};
    dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicated.image = image;
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    if (dedicatedRequirements.requiresDedicatedAllocation || dedicatedRequirements.prefersDedicatedAllocation) allocation.pNext = &dedicated;
    allocation.allocationSize = requirements.memoryRequirements.size;
    allocation.memoryTypeIndex = type;
    Check(functions.allocateMemory(context.GetDevice(), &allocation, nullptr, &memory), "Allocate Vulkan image memory");
    Check(functions.bindImageMemory(context.GetDevice(), image, memory, 0), "Bind Vulkan image memory");
    allocationSize = allocation.allocationSize;
}
