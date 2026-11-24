#include "Output.hpp"
#include <anybox/Unimplemented.hpp>

AnyBox::Graphics::DxgiOutput::DxgiOutput(DxgiAdapter* adapter, ComOwner<IDXGIOutput1> native) : DxgiObject(std::move(native)), adapter(adapter) { adapter->AddRef(); }
AnyBox::Graphics::DxgiOutput::~DxgiOutput() { adapter->Release(); }

HRESULT AnyBox::Graphics::DxgiOutput::GetParent(REFIID interfaceId, void** parent) { return adapter->QueryInterface(interfaceId, parent); }

HRESULT AnyBox::Graphics::DxgiOutput::GetDesc(DXGI_OUTPUT_DESC* description) {
    if (!description) throw std::invalid_argument("DXGI output description is null");
    return check(host->GetDesc(description));
}

HRESULT AnyBox::Graphics::DxgiOutput::WaitForVBlank() { return check(host->WaitForVBlank()); }
HRESULT AnyBox::Graphics::DxgiOutput::GetDisplayModeList(DXGI_FORMAT, UINT, UINT*, DXGI_MODE_DESC*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::FindClosestMatchingMode(const DXGI_MODE_DESC*, DXGI_MODE_DESC*, IUnknown*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::TakeOwnership(IUnknown*, BOOL) { ANYBOX_NOT_IMPLEMENTED(); }
void AnyBox::Graphics::DxgiOutput::ReleaseOwnership() { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::GetGammaControlCapabilities(DXGI_GAMMA_CONTROL_CAPABILITIES*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::SetGammaControl(const DXGI_GAMMA_CONTROL*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::GetGammaControl(DXGI_GAMMA_CONTROL*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::SetDisplaySurface(IDXGISurface*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::GetDisplaySurfaceData(IDXGISurface*) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiOutput::GetFrameStatistics(DXGI_FRAME_STATISTICS*) { ANYBOX_NOT_IMPLEMENTED(); }
void AnyBox::Graphics::DxgiOutput::Unsupported19() { ANYBOX_NOT_IMPLEMENTED(); }
void AnyBox::Graphics::DxgiOutput::Unsupported20() { ANYBOX_NOT_IMPLEMENTED(); }
