#pragma once

#include "Adapter.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid DxgiOutputId{0xAE02EEDB, 0xC735, 0x4690, {0x8D, 0x52, 0x5A, 0x8D, 0xC2, 0x02, 0x13, 0xAA}};

class DxgiOutput final : public DxgiObject<DxgiOutput, IDXGIOutput, IDXGIOutput1> {
public:
    DxgiOutput(DxgiAdapter* adapter, ComOwner<IDXGIOutput1> native);
    ~DxgiOutput();
    static bool Supports(const Guid& id) noexcept { return id == DxgiOutputId; }
    HRESULT GetParent(REFIID interfaceId, void** parent) override;
    HRESULT GetDesc(DXGI_OUTPUT_DESC* description) override;
    HRESULT GetDisplayModeList(DXGI_FORMAT format, UINT flags, UINT* count, DXGI_MODE_DESC* modes) override;
    HRESULT FindClosestMatchingMode(const DXGI_MODE_DESC* mode, DXGI_MODE_DESC* match, IUnknown* device) override;
    HRESULT WaitForVBlank() override;
    HRESULT TakeOwnership(IUnknown* device, BOOL exclusive) override;
    void ReleaseOwnership() override;
    HRESULT GetGammaControlCapabilities(DXGI_GAMMA_CONTROL_CAPABILITIES* capabilities) override;
    HRESULT SetGammaControl(const DXGI_GAMMA_CONTROL* gamma) override;
    HRESULT GetGammaControl(DXGI_GAMMA_CONTROL* gamma) override;
    HRESULT SetDisplaySurface(IDXGISurface* surface) override;
    HRESULT GetDisplaySurfaceData(IDXGISurface* surface) override;
    HRESULT GetFrameStatistics(DXGI_FRAME_STATISTICS* statistics) override;
    virtual void Unsupported19();
    virtual void Unsupported20();
    virtual HRESULT GetDisplayModeList1(DXGI_FORMAT format, UINT flags, UINT* count, DXGI_MODE_DESC1* modes);

private:
    DxgiAdapter* adapter;
};
}
