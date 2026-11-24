#include "TextureViews.hpp"
#include <anybox/Unimplemented.hpp>

void AnyBox::Graphics::CreateTextureShaderView(Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t destination) {
    const auto& metadata = resource.GetMetadata();
    if (metadata.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) ANYBOX_NOT_IMPLEMENTED();
    if ((metadata.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE) != 0) throw std::invalid_argument("Xbox texture does not allow shader resource views");
    auto& texture = resource.GetTexture();
    Descriptor::TextureViewDescription view;
    view.address = texture.GetAddress();
    if (description) {
        if (description->Format != metadata.Format) ANYBOX_NOT_IMPLEMENTED();
        view.componentMapping = description->Shader4ComponentMapping;
        if (description->ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2D) {
            if (description->Texture2D.PlaneSlice != 0 || description->Texture2D.ResourceMinLODClamp != 0) ANYBOX_NOT_IMPLEMENTED();
            view.firstMip = description->Texture2D.MostDetailedMip;
            view.mipCount = description->Texture2D.MipLevels;
            view.layerCount = 1;
        } else if (description->ViewDimension == D3D12_SRV_DIMENSION_TEXTURE2DARRAY) {
            if (description->Texture2DArray.PlaneSlice != 0 || description->Texture2DArray.ResourceMinLODClamp != 0) ANYBOX_NOT_IMPLEMENTED();
            view.firstMip = description->Texture2DArray.MostDetailedMip;
            view.mipCount = description->Texture2DArray.MipLevels;
            view.firstLayer = description->Texture2DArray.FirstArraySlice;
            view.layerCount = description->Texture2DArray.ArraySize;
        } else ANYBOX_NOT_IMPLEMENTED();
    }
    auto words = Descriptor::EncodeTexture2D(texture.GetDescription(), view);
    texture.GetView(words);
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
