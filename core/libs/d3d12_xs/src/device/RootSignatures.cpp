#include "Device.hpp"
#include "../root/Signature.hpp"
#include <d3d12_xs.hpp>
#include <memory>
#include <stdexcept>

namespace {
struct RootDeleter {
    void operator()(ID3D12RootSignature* value) const noexcept { value->Release(); }
};
struct BlobDeleter {
    void operator()(ID3DBlob* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateRootSignature(Device* self, std::uint32_t nodeMask, const void* data, std::size_t size, const Guid& interfaceId, void** signature) {
    if (!signature) throw std::invalid_argument("Root signature output is null");
    *signature = nullptr;
    if (!data || size == 0) throw std::invalid_argument("Root signature bytecode is empty");
    if (size < 32 || std::memcmp(data, "DXBC", 4) != 0) throw std::invalid_argument("Root signature container header is invalid");
    if (!RootSignature::Supports(interfaceId)) throw std::invalid_argument("Unsupported root signature creation interface");
    auto layout = std::make_unique<RootDescription>(std::span(static_cast<const std::byte*>(data), size));
    std::unique_ptr<ID3DBlob, BlobDeleter> hostBlob;
    if ((layout->GetFlags() & XboxRootSignatureFlags) != 0) {
        auto description = layout->GetDescription();
        SetRootSignatureFlags(description, layout->GetFlags() & ~XboxRootSignatureFlags);
        ID3DBlob* output = nullptr;
        D3D12SerializeVersionedRootSignature(&description, &output, nullptr);
        hostBlob.reset(output);
        data = hostBlob->GetBufferPointer();
        size = hostBlob->GetBufferSize();
    }

    void* host = nullptr;
    auto result = self->host->CreateRootSignature(nodeMask, data, size, NativeGuid(RootSignatureId), &host);
    CheckGraphicsResult(result, "ID3D12Device::CreateRootSignature");
    if (!host) throw std::runtime_error("D3D12 root signature creation returned no interface");
    std::unique_ptr<ID3D12RootSignature, RootDeleter> ownedHost(static_cast<ID3D12RootSignature*>(host));
    auto wrapper = std::make_unique<RootSignature>(ownedHost.get(), self, std::move(layout));
    ownedHost.release();
    *signature = wrapper.release();
    return result;
}
