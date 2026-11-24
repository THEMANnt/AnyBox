#include <anybox/vulkan/Context.hpp>
#include <anybox/platform/WindowsError.hpp>
#include <cstring>

AnyBox::Vulkan::Context::Context(std::uint32_t adapterIndex, std::uint32_t queueCount) : Context(AdapterSelection(adapterIndex), queueCount) {}

AnyBox::Vulkan::Context::Context(const AdapterLuid& adapterLuid, std::uint32_t queueCount) : Context(AdapterSelection(adapterLuid), queueCount) {}

AnyBox::Vulkan::Context::Context(const AdapterSelection& selection, std::uint32_t queueCount) {
    if (queueCount == 0) throw std::invalid_argument("Vulkan context requires a queue");
    module = LoadLibraryExW(L"vulkan-1.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) Windows::ThrowLastError("LoadLibraryExW Vulkan loader");
    try {
        auto address = GetProcAddress(module, "vkGetInstanceProcAddr");
        if (!address) Windows::ThrowLastError("GetProcAddress Vulkan loader");
        static_assert(sizeof(getInstanceProcAddr) == sizeof(address));
        std::memcpy(&getInstanceProcAddr, &address, sizeof(address));
        auto version = RequireFunction<PFN_vkEnumerateInstanceVersion>(getInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"), "vkEnumerateInstanceVersion");
        std::uint32_t supported = 0;
        Check(version(&supported), "Query Vulkan instance version");
        if (supported < VK_API_VERSION_1_2) throw std::runtime_error("Vulkan 1.2 is required");
        VkApplicationInfo app{};
        app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app.pApplicationName = "AnyBox";
        app.apiVersion = VK_API_VERSION_1_2;
        VkInstanceCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        info.pApplicationInfo = &app;
        auto create = RequireFunction<PFN_vkCreateInstance>(getInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"), "vkCreateInstance");
        Check(create(&info, nullptr, &instance), "Create Vulkan instance");
        loadInstanceFunctions();
        selectAdapter(selection, queueCount);
        createDevice(queueCount);
        loadDeviceFunctions();
        queues.resize(queueCount);
        queueMutexes = std::make_unique<std::mutex[]>(queueCount);
        for (std::uint32_t index = 0; index < queueCount; ++index) deviceFunctions.getDeviceQueue(device, queueFamily, index, &queues[index]);
    } catch (...) { release(); throw; }
}

AnyBox::Vulkan::Context::~Context() { release(); }

void AnyBox::Vulkan::Context::release() noexcept {
    if (device && deviceFunctions.destroyDevice) deviceFunctions.destroyDevice(device, nullptr);
    if (instance && instanceFunctions.destroyInstance) instanceFunctions.destroyInstance(instance, nullptr);
    if (module) FreeLibrary(module);
}

VkQueue AnyBox::Vulkan::Context::GetQueue(std::uint32_t index) const {
    if (index >= queues.size()) throw std::out_of_range("Vulkan queue index exceeds its configured count");
    return queues[index];
}

VkExternalBufferProperties AnyBox::Vulkan::Context::GetExternalBufferProperties(VkBufferUsageFlags usage) const {
    VkPhysicalDeviceExternalBufferInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_BUFFER_INFO;
    info.usage = usage;
    info.handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_HOST_ALLOCATION_BIT_EXT;
    VkExternalBufferProperties result{};
    result.sType = VK_STRUCTURE_TYPE_EXTERNAL_BUFFER_PROPERTIES;
    instanceFunctions.getExternalBufferProperties(physicalDevice, &info, &result);
    return result;
}
