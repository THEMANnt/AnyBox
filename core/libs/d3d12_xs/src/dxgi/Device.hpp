#pragma once

#include "../host/Dxgi.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid DxgiDeviceId{0x54EC77FA, 0x1377, 0x44E6, {0x8C, 0x32, 0x88, 0xFD, 0x5F, 0x44, 0xC8, 0x4C}};
inline constexpr Guid DxgiObjectId{0xAEC22FB8, 0x76F3, 0x4639, {0x9B, 0xE0, 0x28, 0xEB, 0x43, 0xA6, 0x7A, 0x2E}};

class DxgiDevice final : public IDXGIDevice {
public:
    static DxgiDevice* Acquire(Device* device);
    HRESULT QueryInterface(REFIID interfaceId, void** object) override;
    ULONG AddRef() override;
    ULONG Release() override;
    HRESULT SetPrivateData(REFGUID key, UINT size, const void* data) override;
    HRESULT SetPrivateDataInterface(REFGUID key, const IUnknown* object) override;
    HRESULT GetPrivateData(REFGUID key, UINT* size, void* data) override;
    HRESULT GetParent(REFIID interfaceId, void** parent) override;
    HRESULT GetAdapter(IDXGIAdapter** output) override;
    HRESULT CreateSurface(const DXGI_SURFACE_DESC* description, UINT count, DXGI_USAGE usage, const DXGI_SHARED_RESOURCE* sharedResource, IDXGISurface** surfaces) override;
    HRESULT QueryResourceResidency(IUnknown* const* resources, DXGI_RESIDENCY* residency, UINT count) override;
    HRESULT SetGPUThreadPriority(INT priority) override;
    HRESULT GetGPUThreadPriority(INT* priority) override;

private:
    explicit DxgiDevice(Device* device);
    ~DxgiDevice();
    ULONG tryAddRef();
    std::atomic<std::uint32_t> references{1};
    Device* device;
    NativeAdapter adapter;
};
}
