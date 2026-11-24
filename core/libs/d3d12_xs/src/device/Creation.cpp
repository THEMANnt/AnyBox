#include "Device.hpp"
#include "../host/Library.hpp"
#include "../memory/Configuration.hpp"
#include <d3d12_xs.hpp>
#include <anybox/Unimplemented.hpp>
#include <memory>
#include <stdexcept>

namespace {
struct DeviceDeleter {
    void operator()(ID3D12Device* value) const noexcept { value->Release(); }
};
}

std::int32_t D3D12XboxCreateDevice(IGraphicsUnknown* adapter, const D3D12XboxCreateDeviceParameters* parameters, const AnyBox::Guid& interfaceId, void** device) noexcept(false) {
    if (!device) throw std::invalid_argument("Graphics device output is null");
    *device = nullptr;
    if (!parameters) throw std::invalid_argument("Xbox graphics device parameters are null");
    if (parameters->version != 0x20004 || parameters->processDebugFlags != 0) ANYBOX_NOT_IMPLEMENTED();
    if (!AnyBox::Graphics::Device::Supports(interfaceId)) throw std::invalid_argument("Unsupported Xbox graphics device interface");
    if (adapter) ANYBOX_NOT_IMPLEMENTED();
    static auto create = AnyBox::Graphics::GetHostLibrary().Get<PFN_D3D12_CREATE_DEVICE>("D3D12CreateDevice");
    void* host = nullptr;
    auto result = create(nullptr, D3D_FEATURE_LEVEL_12_0, AnyBox::Graphics::NativeGuid(AnyBox::Graphics::BaseDeviceId), &host);
    AnyBox::Graphics::CheckGraphicsResult(result, "D3D12CreateDevice");
    if (!host) throw std::runtime_error("D3D12CreateDevice returned no interface");
    std::unique_ptr<ID3D12Device, DeviceDeleter> ownedHost(static_cast<ID3D12Device*>(host));
    AnyBox::Graphics::EnsureGraphicsMemoryConfigured();
    *device = AnyBox::Graphics::AcquireDevice(ownedHost.get(), *parameters);
    return static_cast<std::int32_t>(result);
}
