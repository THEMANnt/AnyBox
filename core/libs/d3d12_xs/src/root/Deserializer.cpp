#include "Deserializer.hpp"
#include <stdexcept>
#include <system_error>

AnyBox::Graphics::RootDeserializer::RootDeserializer(ID3D12RootSignatureDeserializer* host, std::uint32_t flags) : host(host), description(*host->GetRootSignatureDesc()) {
    description.Flags = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags);
}

AnyBox::Graphics::RootDeserializer::~RootDeserializer() {
    host->Release();
}

HRESULT AnyBox::Graphics::RootDeserializer::QueryInterface(REFIID interfaceId, void** object) {
    if (!object) throw std::invalid_argument("Root signature deserializer interface output is null");
    *object = nullptr;
    constexpr GUID unknownId{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
    if (interfaceId != RootDeserializerId && interfaceId != unknownId) throw std::system_error(static_cast<int>(E_NOINTERFACE), std::system_category(), "Root signature deserializer interface is unsupported");
    AddRef();
    *object = static_cast<ID3D12RootSignatureDeserializer*>(this);
    return S_OK;
}

ULONG AnyBox::Graphics::RootDeserializer::AddRef() {
    return references.fetch_add(1, std::memory_order_relaxed) + 1;
}

ULONG AnyBox::Graphics::RootDeserializer::Release() {
    auto count = references.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0) delete this;
    return count;
}

const D3D12_ROOT_SIGNATURE_DESC* AnyBox::Graphics::RootDeserializer::GetRootSignatureDesc() {
    return &description;
}
