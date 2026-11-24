#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <atomic>
#include <cstdint>

namespace AnyBox::Graphics {
inline constexpr GUID RootDeserializerId{0x34AB647B, 0x3CC8, 0x46AC, {0x84, 0x1B, 0xC0, 0x96, 0x56, 0x45, 0xC0, 0x46}};
class RootDeserializer final : public ID3D12RootSignatureDeserializer {
public:
    RootDeserializer(ID3D12RootSignatureDeserializer* host, std::uint32_t flags);
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID interfaceId, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;
    const D3D12_ROOT_SIGNATURE_DESC* STDMETHODCALLTYPE GetRootSignatureDesc() override;

private:
    ~RootDeserializer();
    std::atomic<ULONG> references{1};
    ID3D12RootSignatureDeserializer* host;
    D3D12_ROOT_SIGNATURE_DESC description;
};
}
