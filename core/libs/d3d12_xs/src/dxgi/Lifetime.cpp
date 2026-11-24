#include "Device.hpp"
#include "Adapter.hpp"
#include <limits>
#include <stdexcept>

AnyBox::Graphics::DxgiDevice::DxgiDevice(Device* device) : device(device), adapter(new DxgiAdapter(GetHostAdapter(device))) { Device::AddRef(device); }

AnyBox::Graphics::DxgiDevice::~DxgiDevice() {
    {
        std::scoped_lock lock(device->dxgiMutex);
        if (device->dxgiDevice == this) device->dxgiDevice = nullptr;
    }
    adapter.reset();
    Device::Release(device);
}

AnyBox::Graphics::DxgiDevice* AnyBox::Graphics::DxgiDevice::Acquire(Device* device) {
    std::scoped_lock lock(device->dxgiMutex);
    if (device->dxgiDevice && device->dxgiDevice->tryAddRef() != 0) return device->dxgiDevice;
    device->dxgiDevice = nullptr;
    auto* value = new DxgiDevice(device);
    device->dxgiDevice = value;
    return value;
}

HRESULT AnyBox::Graphics::DxgiDevice::QueryInterface(REFIID interfaceId, void** object) {
    Guid id;
    static_assert(sizeof(id) == sizeof(interfaceId));
    std::memcpy(&id, &interfaceId, sizeof(id));
    return Device::QueryInterface(device, id, object);
}

ULONG AnyBox::Graphics::DxgiDevice::tryAddRef() {
    auto previous = references.load(std::memory_order_relaxed);
    do {
        if (previous == 0) return 0;
        if (previous == std::numeric_limits<std::uint32_t>::max()) throw std::overflow_error("DXGI device reference count overflows");
    } while (!references.compare_exchange_weak(previous, previous + 1, std::memory_order_relaxed));
    return previous + 1;
}

ULONG AnyBox::Graphics::DxgiDevice::AddRef() {
    auto result = tryAddRef();
    if (result == 0) throw std::logic_error("DXGI device has already been released");
    return result;
}

ULONG AnyBox::Graphics::DxgiDevice::Release() {
    auto previous = references.fetch_sub(1, std::memory_order_acq_rel);
    if (previous == 0) throw std::logic_error("DXGI device reference count underflows");
    if (previous == 1) delete this;
    return previous - 1;
}
