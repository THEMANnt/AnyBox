#include "../device/Device.hpp"
#include "../resource/Resource.hpp"
#include "RawBuffer.hpp"
#include "TextureViews.hpp"
#include <anybox/Unimplemented.hpp>
#include <cstddef>

static_assert(sizeof(D3D12_UNORDERED_ACCESS_VIEW_DESC) == 40);
static_assert(offsetof(D3D12_UNORDERED_ACCESS_VIEW_DESC, Buffer) == 8);
static_assert(offsetof(D3D12_UNORDERED_ACCESS_VIEW_DESC, Buffer) + offsetof(D3D12_BUFFER_UAV, StructureByteStride) == 20);
static_assert(offsetof(D3D12_UNORDERED_ACCESS_VIEW_DESC, Buffer) + offsetof(D3D12_BUFFER_UAV, Flags) == 32);

void AnyBox::Graphics::Device::CreateUnorderedAccessView(Device* self, void* resource, void* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox unordered view destination is null or misaligned");
    if (resource) {
        auto* object = Resource::From(resource, self);
        const auto& metadata = object->GetMetadata();
        if (metadata.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) {
            CreateTextureUnorderedView(*object, counterResource, description, destination);
            return;
        }
        if (!description || description->ViewDimension != D3D12_UAV_DIMENSION_BUFFER || description->Format != DXGI_FORMAT_R32_TYPELESS || description->Buffer.Flags != D3D12_BUFFER_UAV_FLAG_RAW) ANYBOX_NOT_IMPLEMENTED();
        if (counterResource || description->Buffer.CounterOffsetInBytes != 0 || description->Buffer.StructureByteStride != 0) throw std::invalid_argument("Xbox raw unordered view cannot have a counter or structure stride");
        if ((metadata.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0) throw std::invalid_argument("Xbox resource does not allow unordered access");
        constexpr auto mapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 4, 4, 5);
        auto words = EncodeRawBuffer(Resource::GetGPUVirtualAddress(object), metadata.Width, description->Buffer.FirstElement, description->Buffer.NumElements, mapping);
        std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
        return;
    }
    std::array<std::uint64_t, 4> words{};
    if (description && description->ViewDimension == D3D12_UAV_DIMENSION_BUFFER && static_cast<std::uint32_t>(description->Buffer.Flags) == 2 && description->Buffer.StructureByteStride != 0 && description->Format != DXGI_FORMAT_UNKNOWN) {
        auto offset = static_cast<std::uint32_t>(description->Buffer.FirstElement);
        auto stride = static_cast<std::uint64_t>(description->Buffer.StructureByteStride & 0x3FFF) << 48;
        words[0] = offset | stride;
    }
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
