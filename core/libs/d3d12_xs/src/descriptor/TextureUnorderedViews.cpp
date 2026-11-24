#include "TextureViews.hpp"
#include <anybox/Unimplemented.hpp>

void AnyBox::Graphics::CreateTextureUnorderedView(Resource& resource, void* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* description, std::uint64_t destination) {
    const auto& metadata = resource.GetMetadata();
    if (metadata.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) ANYBOX_NOT_IMPLEMENTED();
    if (counterResource) throw std::invalid_argument("Texture unordered view cannot have a counter resource");
    if ((metadata.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0) throw std::invalid_argument("Xbox texture does not allow unordered access");
    auto& texture = resource.GetTexture();
    Descriptor::StorageViewDescription view{texture.GetAddress(), 0, 0, UINT32_MAX};
    if (description) {
        if (description->Format != DXGI_FORMAT_UNKNOWN && description->Format != metadata.Format) ANYBOX_NOT_IMPLEMENTED();
        if (description->ViewDimension == D3D12_UAV_DIMENSION_TEXTURE2D) {
            if (description->Texture2D.PlaneSlice != 0) ANYBOX_NOT_IMPLEMENTED();
            view.mip = description->Texture2D.MipSlice;
            view.layerCount = 1;
        } else if (description->ViewDimension == D3D12_UAV_DIMENSION_TEXTURE2DARRAY) {
            if (description->Texture2DArray.PlaneSlice != 0) ANYBOX_NOT_IMPLEMENTED();
            view.mip = description->Texture2DArray.MipSlice;
            view.firstLayer = description->Texture2DArray.FirstArraySlice;
            view.layerCount = description->Texture2DArray.ArraySize;
        } else ANYBOX_NOT_IMPLEMENTED();
    }
    auto words = Descriptor::EncodeStorageTexture2D(texture.GetDescription(), view);
    texture.GetStorageView(words);
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
