#pragma once

#include "Object.hpp"

namespace AnyBox::Graphics {
inline constexpr Guid DxgiAdapterId{0x2411E7E1, 0x12AC, 0x4CCF, {0xBD, 0x14, 0x97, 0x98, 0xE8, 0x53, 0x4D, 0xC0}};
inline constexpr Guid DxgiAdapter1Id{0x29038F61, 0x3839, 0x4626, {0x91, 0xFD, 0x08, 0x68, 0x79, 0x01, 0x1A, 0x05}};

class DxgiAdapter final : public DxgiObject<DxgiAdapter, IDXGIAdapter1, IDXGIAdapter1> {
public:
    explicit DxgiAdapter(NativeAdapter native);
    static bool Supports(const Guid& id) noexcept { return id == DxgiAdapterId || id == DxgiAdapter1Id; }
    HRESULT GetParent(REFIID interfaceId, void** parent) override;
    HRESULT EnumOutputs(UINT index, IDXGIOutput** output) override;
    HRESULT GetDesc(DXGI_ADAPTER_DESC* description) override;
    HRESULT CheckInterfaceSupport(REFGUID interfaceId, LARGE_INTEGER* version) override;
    HRESULT GetDesc1(DXGI_ADAPTER_DESC1* description) override;
};
}
