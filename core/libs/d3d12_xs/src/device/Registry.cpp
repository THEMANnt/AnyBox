#include "Device.hpp"
#include <memory>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

namespace {
struct Registry {
    std::mutex mutex;
    std::unordered_map<ID3D12Device*, AnyBox::Graphics::Device*> devices;
};

Registry& GetRegistry() {
    static Registry registry;
    return registry;
}
}

AnyBox::Graphics::Device* AnyBox::Graphics::AcquireDevice(ID3D12Device* host, const D3D12XboxCreateDeviceParameters& parameters) {
    auto& registry = GetRegistry();
    std::unique_lock lock(registry.mutex);
    auto found = registry.devices.find(host);
    if (found != registry.devices.end()) {
        auto* device = found->second;
        if (Device::TryAddRef(device) != 0) {
            if (device->Matches(parameters)) return device;
            lock.unlock();
            Device::Release(device);
            throw std::logic_error("Existing graphics device parameters differ from the creation request");
        }
        registry.devices.erase(found);
    }
    auto result = std::make_unique<Device>(host, parameters);
    registry.devices.emplace(host, result.get());
    return result.release();
}

void AnyBox::Graphics::ForgetDevice(Device* device) {
    auto& registry = GetRegistry();
    std::lock_guard lock(registry.mutex);
    auto found = registry.devices.find(device->GetHost());
    if (found != registry.devices.end() && found->second == device) registry.devices.erase(found);
}
