#ifndef ANYBOX_D3D12_XS_HPP
#define ANYBOX_D3D12_XS_HPP

#include <d3d12_xsTypes.hpp>
#include <anybox/Types.hpp>

#include <cstddef>
#include <cstdint>

extern "C" {

std::int32_t D3D12ConfigureVirtualMemoryX(D3D12VirtualMemoryConfiguration* configuration) noexcept(false);
long D3D12CreateRootSignatureDeserializer(const void* sourceData, std::size_t sourceDataSize, const _GUID& interfaceId, void** deserializer) noexcept(false);
long D3D12SerializeVersionedRootSignature(const D3D12VersionedRootSignatureDescription* description, ID3DBlob** blob, ID3DBlob** errorBlob) noexcept(false);
std::int32_t D3D12XboxCreateDevice(IGraphicsUnknown* adapter, const D3D12XboxCreateDeviceParameters* parameters, const AnyBox::Guid& interfaceId, void** device) noexcept(false);

}

#endif
