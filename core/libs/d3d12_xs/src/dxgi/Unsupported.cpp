#include "Device.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::DxgiDevice::CreateSurface(const DXGI_SURFACE_DESC*, UINT, DXGI_USAGE, const DXGI_SHARED_RESOURCE*, IDXGISurface**) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiDevice::QueryResourceResidency(IUnknown* const*, DXGI_RESIDENCY*, UINT) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiDevice::SetGPUThreadPriority(INT) { ANYBOX_NOT_IMPLEMENTED(); }
HRESULT AnyBox::Graphics::DxgiDevice::GetGPUThreadPriority(INT*) { ANYBOX_NOT_IMPLEMENTED(); }
