#include "Device.hpp"

HRESULT AnyBox::Graphics::Device::GetDeviceRemovedReason(Device* self) {
    auto result = self->host->GetDeviceRemovedReason();
    CheckGraphicsResult(result, "ID3D12Device::GetDeviceRemovedReason");
    return result;
}

LUID AnyBox::Graphics::Device::GetAdapterLuid(Device* self) { return self->host->GetAdapterLuid(); }
