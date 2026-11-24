#include "Adapter.hpp"
#include "Output.hpp"

AnyBox::Graphics::DxgiAdapter::DxgiAdapter(NativeAdapter native) : DxgiObject(std::move(native)) {}

HRESULT AnyBox::Graphics::DxgiAdapter::GetParent(REFIID interfaceId, void** parent) {
    if (!parent) throw std::invalid_argument("DXGI factory output is null");
    *parent = nullptr;
    return check(host->GetParent(interfaceId, parent));
}

HRESULT AnyBox::Graphics::DxgiAdapter::EnumOutputs(UINT index, IDXGIOutput** output) {
    if (!output) throw std::invalid_argument("DXGI output destination is null");
    *output = nullptr;
    IDXGIOutput* value = nullptr;
    auto result = host->EnumOutputs(index, &value);
    if (result == DXGI_ERROR_NOT_FOUND) return result;
    check(result);
    ComOwner<IDXGIOutput> owner(value);
    constexpr GUID id{0x00CDDEA8, 0x939B, 0x4B83, {0xA3, 0x40, 0xA6, 0x85, 0x22, 0x66, 0x66, 0xCC}};
    void* extended = nullptr;
    check(owner->QueryInterface(id, &extended));
    ComOwner<IDXGIOutput1> native(static_cast<IDXGIOutput1*>(extended));
    *output = new DxgiOutput(this, std::move(native));
    return S_OK;
}

HRESULT AnyBox::Graphics::DxgiAdapter::GetDesc(DXGI_ADAPTER_DESC* description) {
    if (!description) throw std::invalid_argument("DXGI adapter description output is null");
    return check(host->GetDesc(description));
}

HRESULT AnyBox::Graphics::DxgiAdapter::GetDesc1(DXGI_ADAPTER_DESC1* description) {
    if (!description) throw std::invalid_argument("DXGI adapter description output is null");
    return check(host->GetDesc1(description));
}

HRESULT AnyBox::Graphics::DxgiAdapter::CheckInterfaceSupport(REFGUID interfaceId, LARGE_INTEGER* version) { return check(host->CheckInterfaceSupport(interfaceId, version)); }
