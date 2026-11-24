#include "Container.hpp"
#include "Serialized.hpp"
#include <anybox/graphics/container/Dxbc.hpp>
#include <algorithm>
#include <span>
#include <stdexcept>

std::uint32_t AnyBox::Graphics::GetRootSignatureFlags(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& description) {
    if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_0) return description.Desc_1_0.Flags;
    if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_1) return description.Desc_1_1.Flags;
    throw std::invalid_argument("Unsupported Xbox root signature version");
}

void AnyBox::Graphics::SetRootSignatureFlags(D3D12_VERSIONED_ROOT_SIGNATURE_DESC& description, std::uint32_t flags) {
    auto value = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(flags);
    if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_0) description.Desc_1_0.Flags = value;
    else if (description.Version == D3D_ROOT_SIGNATURE_VERSION_1_1) description.Desc_1_1.Flags = value;
    else throw std::invalid_argument("Unsupported Xbox root signature version");
}

void AnyBox::Graphics::SetSerializedRootFlags(ID3DBlob& blob, std::uint32_t flags) {
    auto bytes = std::span(static_cast<std::byte*>(blob.GetBufferPointer()), blob.GetBufferSize());
    auto offset = LocateSerializedRootFlags(bytes);
    for (std::size_t index = 0; index < 4; ++index) bytes[offset + index] = static_cast<std::byte>(flags >> (index * 8));
    auto checksum = Container::ComputeDxbcChecksum(bytes.subspan(20));
    std::ranges::copy(checksum, bytes.begin() + 4);
}
