#include "Fence.hpp"
#include <limits>

std::uint64_t AnyBox::Graphics::Fence::GetCompletedValue(Fence* self) {
    auto value = self->host->GetCompletedValue();
    if (value == std::numeric_limits<std::uint64_t>::max()) CheckGraphicsResult(self->device->GetHost()->GetDeviceRemovedReason(), "ID3D12Device::GetDeviceRemovedReason");
    return value;
}

HRESULT AnyBox::Graphics::Fence::SetEventOnCompletion(Fence* self, std::uint64_t value, HANDLE event) {
    auto result = self->host->SetEventOnCompletion(value, event);
    CheckGraphicsResult(result, "ID3D12Fence::SetEventOnCompletion");
    return result;
}

HRESULT AnyBox::Graphics::Fence::Signal(Fence* self, std::uint64_t value) {
    auto result = self->host->Signal(value);
    CheckGraphicsResult(result, "ID3D12Fence::Signal");
    return result;
}

D3D12_FENCE_FLAGS AnyBox::Graphics::Fence::GetCreationFlags(Fence* self) { return self->flags; }
