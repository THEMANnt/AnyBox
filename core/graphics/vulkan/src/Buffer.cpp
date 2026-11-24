#include <anybox/vulkan/Buffer.hpp>
#include <limits>
#include <utility>

AnyBox::Vulkan::Buffer::Buffer(Context& context, VkDeviceSize size, VkBufferUsageFlags usage) : context(context), size(size), usage(usage) {
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() || usage == 0) throw std::invalid_argument("Vulkan buffer size or usage is invalid");
}

AnyBox::Vulkan::Buffer AnyBox::Vulkan::Buffer::Import(Context& context, std::span<std::byte> backing, VkDeviceSize size, VkBufferUsageFlags usage) {
    auto alignment = context.GetHostPointerAlignment();
    if (backing.empty() || size > backing.size() || alignment == 0 || reinterpret_cast<std::uintptr_t>(backing.data()) % alignment != 0 || backing.size() % alignment != 0) throw std::invalid_argument("Vulkan imported buffer backing is empty, undersized or misaligned");
    Buffer result(context, size, usage);
    result.initialize(backing, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    return result;
}

AnyBox::Vulkan::Buffer AnyBox::Vulkan::Buffer::Allocate(Context& context, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) {
    Buffer result(context, size, usage);
    result.initialize({}, properties);
    return result;
}

AnyBox::Vulkan::Buffer::Buffer(Buffer&& other) noexcept : context(other.context), size(other.size), usage(other.usage), buffer(std::exchange(other.buffer, VK_NULL_HANDLE)), memory(std::exchange(other.memory, VK_NULL_HANDLE)), properties(other.properties), imported(std::exchange(other.imported, nullptr)), mapped(std::exchange(other.mapped, nullptr)) {}

AnyBox::Vulkan::Buffer::~Buffer() {
    const auto& functions = context.GetFunctions();
    if (mapped) functions.unmapMemory(context.GetDevice(), memory);
    if (buffer) functions.destroyBuffer(context.GetDevice(), buffer, nullptr);
    if (memory) functions.freeMemory(context.GetDevice(), memory, nullptr);
}

VkDeviceAddress AnyBox::Vulkan::Buffer::GetAddress() const {
    if (!buffer || (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) == 0) throw std::logic_error("Vulkan buffer has no device address usage");
    VkBufferDeviceAddressInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    info.buffer = buffer;
    auto value = context.GetFunctions().getBufferAddress(context.GetDevice(), &info);
    if (value == 0) throw std::runtime_error("Vulkan buffer device address is zero");
    return value;
}

std::span<std::byte> AnyBox::Vulkan::Buffer::Map() {
    if (!memory || (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) throw std::logic_error("Vulkan buffer memory is not host visible");
    if (imported) return {imported, static_cast<std::size_t>(size)};
    if (!mapped) {
        void* output = nullptr;
        Check(context.GetFunctions().mapMemory(context.GetDevice(), memory, 0, VK_WHOLE_SIZE, 0, &output), "Map Vulkan buffer memory");
        if (!output) throw std::runtime_error("Vulkan buffer mapping is null");
        mapped = static_cast<std::byte*>(output);
    }
    return {mapped, static_cast<std::size_t>(size)};
}
