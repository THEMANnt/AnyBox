#include <anybox/vulkan/Buffer.hpp>

std::uint32_t AnyBox::Vulkan::Buffer::chooseMemoryType(std::uint32_t bits, VkMemoryPropertyFlags required) {
    const auto& memoryProperties = context.GetMemoryProperties();
    for (std::uint32_t index = 0; index < memoryProperties.memoryTypeCount; ++index) {
        auto value = memoryProperties.memoryTypes[index].propertyFlags;
        if ((bits & (1u << index)) != 0 && (value & required) == required) {
            properties = value;
            return index;
        }
    }
    throw std::runtime_error("Vulkan buffer has no compatible memory type with the requested properties");
}

void AnyBox::Vulkan::Buffer::initialize(std::span<std::byte> backing, VkMemoryPropertyFlags required) {
    const auto& functions = context.GetFunctions();
    VkExternalMemoryBufferCreateInfo external{};
    external.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    external.handleTypes = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
    VkExternalMemoryProperties externalProperties{};
    if (!backing.empty()) {
        externalProperties = context.GetExternalBufferProperties(usage).externalMemoryProperties;
        if ((externalProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_IMPORTABLE_BIT) == 0) throw std::runtime_error("Vulkan host allocation imports are unsupported for this buffer usage");
    }
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.pNext = backing.empty() ? nullptr : &external;
    info.size = backing.empty() ? size : backing.size();
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    Check(functions.createBuffer(context.GetDevice(), &info, nullptr, &buffer), "Create Vulkan buffer");
    VkBufferMemoryRequirementsInfo2 requirementsInfo{};
    requirementsInfo.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
    requirementsInfo.buffer = buffer;
    VkMemoryDedicatedRequirements dedicatedRequirements{};
    dedicatedRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_REQUIREMENTS;
    VkMemoryRequirements2 requirements{};
    requirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
    requirements.pNext = &dedicatedRequirements;
    functions.getBufferRequirements(context.GetDevice(), &requirementsInfo, &requirements);
    auto bits = requirements.memoryRequirements.memoryTypeBits;
    if (!backing.empty()) {
        if (requirements.memoryRequirements.size > backing.size() || requirements.memoryRequirements.size % context.GetHostPointerAlignment() != 0) throw std::length_error("Vulkan buffer allocation requirements do not fit the imported host backing");
        VkMemoryHostPointerPropertiesEXT pointerProperties{};
        pointerProperties.sType = VK_STRUCTURE_TYPE_MEMORY_HOST_POINTER_PROPERTIES_EXT;
        Check(functions.getHostPointerProperties(context.GetDevice(), VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT, backing.data(), &pointerProperties), "Query Vulkan host memory types");
        bits &= pointerProperties.memoryTypeBits;
    }
    VkMemoryAllocateFlagsInfo flags{};
    flags.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    if ((usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0) flags.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;
    VkImportMemoryHostPointerInfoEXT import{};
    import.sType = VK_STRUCTURE_TYPE_IMPORT_MEMORY_HOST_POINTER_INFO_EXT;
    import.pNext = &flags;
    import.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
    import.pHostPointer = backing.data();
    VkMemoryDedicatedAllocateInfo dedicated{};
    dedicated.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
    dedicated.pNext = backing.empty() ? static_cast<const void*>(&flags) : &import;
    bool requiresDedicated = dedicatedRequirements.requiresDedicatedAllocation || (externalProperties.externalMemoryFeatures & VK_EXTERNAL_MEMORY_FEATURE_DEDICATED_ONLY_BIT) != 0;
    if (requiresDedicated) dedicated.buffer = buffer;
    VkMemoryAllocateInfo allocation{};
    allocation.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocation.pNext = requiresDedicated ? static_cast<const void*>(&dedicated) : dedicated.pNext;
    allocation.allocationSize = requirements.memoryRequirements.size;
    allocation.memoryTypeIndex = chooseMemoryType(bits, required);
    Check(functions.allocateMemory(context.GetDevice(), &allocation, nullptr, &memory), "Allocate Vulkan buffer memory");
    Check(functions.bindBufferMemory(context.GetDevice(), buffer, memory, 0), "Bind Vulkan buffer memory");
    imported = backing.data();
}
