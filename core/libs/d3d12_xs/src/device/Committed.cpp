#include "Device.hpp"
#include "../resource/Resource.hpp"
#include "../resource/Texture.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::Device::CreateCommittedResource3(Device* self, const D3D12_HEAP_PROPERTIES* properties, D3D12_HEAP_FLAGS flags, const D3D12_RESOURCE_DESC1* description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue, void* protectedSession, std::uint32_t formatCount, const DXGI_FORMAT* formats, const Guid& interfaceId, void** resource) {
    if (!resource) throw std::invalid_argument("Committed resource output is null");
    *resource = nullptr;
    if (!self || !properties || !description) throw std::invalid_argument("Committed resource creation arguments are null");
    if (!Resource::Supports(interfaceId)) throw std::invalid_argument("Committed resource interface is unsupported");
    if (protectedSession || formatCount != 0 || formats) ANYBOX_NOT_IMPLEMENTED();
    auto backing = CreateTextureBacking(self, *properties, flags, *description, layout, clearValue);
    auto wrapper = std::make_unique<Resource>(std::move(backing), self, *description);
    *resource = wrapper.release();
    return S_OK;
}
