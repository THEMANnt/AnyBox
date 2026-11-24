#include <anybox/vulkan/Timeline.hpp>
#include <utility>

AnyBox::Vulkan::Timeline::Timeline(Context& context, std::uint64_t initialValue) : context(context) {
    VkSemaphoreTypeCreateInfo type{};
    type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    type.initialValue = initialValue;
    VkSemaphoreCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext = &type;
    Check(context.GetFunctions().createSemaphore(context.GetDevice(), &info, nullptr, &semaphore), "Create Vulkan timeline semaphore");
}

AnyBox::Vulkan::Timeline AnyBox::Vulkan::Timeline::ImportD3D12Fence(Context& context, HANDLE handle) {
    if (!handle || handle == INVALID_HANDLE_VALUE) throw std::invalid_argument("D3D12 fence import handle is invalid");
    auto properties = context.GetD3D12FenceProperties();
    if ((properties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT) == 0) throw std::runtime_error("Vulkan cannot import D3D12 fences as timeline semaphores");
    Timeline timeline(context);
    VkImportSemaphoreWin32HandleInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_IMPORT_SEMAPHORE_WIN32_HANDLE_INFO_KHR;
    info.semaphore = timeline.semaphore;
    info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT;
    info.handle = handle;
    Check(context.GetFunctions().importSemaphore(context.GetDevice(), &info), "Import D3D12 fence into Vulkan");
    return timeline;
}

VkExternalSemaphoreProperties AnyBox::Vulkan::Context::GetD3D12FenceProperties() const {
    VkSemaphoreTypeCreateInfo type{};
    type.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
    type.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
    VkPhysicalDeviceExternalSemaphoreInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO;
    info.pNext = &type;
    info.handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_D3D12_FENCE_BIT;
    VkExternalSemaphoreProperties result{};
    result.sType = VK_STRUCTURE_TYPE_EXTERNAL_SEMAPHORE_PROPERTIES;
    instanceFunctions.getExternalSemaphoreProperties(physicalDevice, &info, &result);
    return result;
}

AnyBox::Vulkan::Timeline::Timeline(Timeline&& other) noexcept : context(other.context), semaphore(std::exchange(other.semaphore, VK_NULL_HANDLE)) {}

AnyBox::Vulkan::Timeline::~Timeline() {
    if (semaphore) context.GetFunctions().destroySemaphore(context.GetDevice(), semaphore, nullptr);
}

std::uint64_t AnyBox::Vulkan::Timeline::GetValue() const {
    if (!semaphore) throw std::logic_error("Vulkan timeline semaphore is empty");
    std::uint64_t value = 0;
    Check(context.GetFunctions().getSemaphoreValue(context.GetDevice(), semaphore, &value), "Query Vulkan timeline semaphore");
    return value;
}

bool AnyBox::Vulkan::Timeline::Wait(std::uint64_t value, std::uint64_t timeoutNanoseconds) const {
    if (!semaphore) throw std::logic_error("Vulkan timeline semaphore is empty");
    VkSemaphoreWaitInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
    info.semaphoreCount = 1;
    info.pSemaphores = &semaphore;
    info.pValues = &value;
    auto result = context.GetFunctions().waitSemaphores(context.GetDevice(), &info, timeoutNanoseconds);
    if (result == VK_TIMEOUT) return false;
    Check(result, "Wait Vulkan timeline semaphore");
    return true;
}
