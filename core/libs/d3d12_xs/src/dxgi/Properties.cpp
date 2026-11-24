#include "Device.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <stdexcept>

HRESULT AnyBox::Graphics::DxgiDevice::SetPrivateData(REFGUID key, UINT size, const void* data) {
    if (size != 0 && !data) throw std::invalid_argument("DXGI private data is null");
    auto result = device->GetHost()->SetPrivateData(key, size, data);
    Windows::CheckHResult(result, "DXGI device private data write");
    return result;
}

HRESULT AnyBox::Graphics::DxgiDevice::SetPrivateDataInterface(REFGUID key, const IUnknown* object) {
    auto result = device->GetHost()->SetPrivateDataInterface(key, object);
    Windows::CheckHResult(result, "DXGI device private interface write");
    return result;
}

HRESULT AnyBox::Graphics::DxgiDevice::GetPrivateData(REFGUID key, UINT* size, void* data) {
    if (!size) throw std::invalid_argument("DXGI private data size output is null");
    auto result = device->GetHost()->GetPrivateData(key, size, data);
    Windows::CheckHResult(result, "DXGI device private data read");
    return result;
}

HRESULT AnyBox::Graphics::DxgiDevice::GetParent(REFIID interfaceId, void** parent) {
    if (!parent) throw std::invalid_argument("DXGI parent output is null");
    *parent = nullptr;
    auto result = adapter->QueryInterface(interfaceId, parent);
    Windows::CheckHResult(result, "DXGI device parent query");
    return result;
}

HRESULT AnyBox::Graphics::DxgiDevice::GetAdapter(IDXGIAdapter** output) {
    if (!output) throw std::invalid_argument("DXGI adapter output is null");
    adapter->AddRef();
    *output = adapter.get();
    return S_OK;
}
