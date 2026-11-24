#include "Device.hpp"
#include "../dxgi/Device.hpp"
#include "../frame/Clock.hpp"
#include <anybox/vulkan/Context.hpp>
#include <anybox/vulkan/Buffer.hpp>
#include <algorithm>
#include <limits>
#include <stdexcept>

AnyBox::Graphics::Device::~Device() { delete frameClock; delete nullConstantBuffer; delete gpuContext; host->Release(); }

bool AnyBox::Graphics::Device::Supports(const Guid& interfaceId) noexcept { return interfaceId == UnknownId || interfaceId == ObjectId || std::ranges::find(DeviceInterfaces, interfaceId) != DeviceInterfaces.end(); }

std::int32_t AnyBox::Graphics::Device::QueryInterface(Device* self, const Guid& interfaceId, void** object) {
    if (!object) throw std::invalid_argument("Graphics device interface output is null");
    *object = nullptr;
    if (interfaceId == DxgiDeviceId || interfaceId == DxgiObjectId) {
        *object = DxgiDevice::Acquire(self);
        return 0;
    }
    if (!Supports(interfaceId)) throw std::invalid_argument("Unsupported graphics device interface");
    AddRef(self);
    *object = self;
    return 0;
}

std::uint32_t AnyBox::Graphics::Device::AddRef(Device* self) {
    auto result = TryAddRef(self);
    if (result == 0) throw std::logic_error("Graphics device has already been released");
    return result;
}

std::uint32_t AnyBox::Graphics::Device::TryAddRef(Device* self) {
    auto previous = self->references.load(std::memory_order_relaxed);
    do {
        if (previous == 0) return 0;
        if (previous == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("Graphics device reference count is invalid");
    } while (!self->references.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
    return previous + 1;
}

std::uint32_t AnyBox::Graphics::Device::Release(Device* self) {
    auto previous = self->references.fetch_sub(1, std::memory_order_acq_rel);
    if (previous == 0) throw std::logic_error("Graphics device reference count underflows");
    if (previous == 1) {
        ForgetDevice(self);
        delete self;
    }
    return previous - 1;
}

bool AnyBox::Graphics::Device::Matches(const D3D12XboxCreateDeviceParameters& value) const noexcept { return parameters.version == value.version && parameters.processDebugFlags == value.processDebugFlags && parameters.driverParameters == value.driverParameters; }
