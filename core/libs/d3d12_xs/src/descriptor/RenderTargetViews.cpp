#include "RenderTarget.hpp"
#include <anybox/Unimplemented.hpp>

void AnyBox::Graphics::Device::CreateRenderTargetView(Device* self, void* resource, const D3D12_RENDER_TARGET_VIEW_DESC* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox render target view destination is null or misaligned");
    auto* output = reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination));
    if (!resource) { std::memset(output, 0, sizeof(RenderTargetView)); return; }
    auto* object = Resource::From(resource, self);
    const auto& metadata = object->GetMetadata();
    if (metadata.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) ANYBOX_NOT_IMPLEMENTED();
    if ((metadata.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) == 0) throw std::invalid_argument("Xbox resource does not allow render target views");
    auto& texture = object->GetTexture();
    Descriptor::ColorTargetViewDescription view{texture.GetAddress(), 0, 0, UINT32_MAX};
    if (description) {
        if (description->Format != DXGI_FORMAT_UNKNOWN && description->Format != metadata.Format) ANYBOX_NOT_IMPLEMENTED();
        if (description->ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2D) {
            if (description->Texture2D.PlaneSlice != 0) ANYBOX_NOT_IMPLEMENTED();
            view.mip = description->Texture2D.MipSlice;
            view.layerCount = 1;
        } else if (description->ViewDimension == D3D12_RTV_DIMENSION_TEXTURE2DARRAY) {
            if (description->Texture2DArray.PlaneSlice != 0) ANYBOX_NOT_IMPLEMENTED();
            view.mip = description->Texture2DArray.MipSlice;
            view.firstLayer = description->Texture2DArray.FirstArraySlice;
            view.layerCount = description->Texture2DArray.ArraySize;
        } else ANYBOX_NOT_IMPLEMENTED();
    }
    auto words = Descriptor::EncodeColorTarget(texture.GetDescription(), view, object->GetTexturePitch());
    Descriptor::TextureViewDescription selected;
    selected.address = view.address;
    selected.firstMip = view.mip;
    selected.mipCount = 1;
    selected.firstLayer = view.firstLayer;
    selected.layerCount = view.layerCount;
    texture.GetView(Descriptor::EncodeTexture2D(texture.GetDescription(), selected));
    RenderTargetView value{&GetRenderTargetTable(), static_cast<std::uint32_t>(metadata.Format), 0, object, words};
    std::memcpy(output, &value, sizeof(value));
}
