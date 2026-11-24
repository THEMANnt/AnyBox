#include "Device.hpp"
#include "../resource/Resource.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::Device::CreatePlacedResourceLayoutX(Device* self, std::uintptr_t address, const D3D12_RESOURCE_DESC1* description, D3D12_BARRIER_LAYOUT layout, const D3D12_CLEAR_VALUE* clearValue, const Guid& interfaceId, void** resource) {
    if (!resource) throw std::invalid_argument("Placed resource output is null");
    *resource = nullptr;
    if (!description) throw std::invalid_argument("Placed resource description is null");
    if (!Resource::Supports(interfaceId)) throw std::invalid_argument("Unsupported placed resource interface");
    if (description->Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) ANYBOX_NOT_IMPLEMENTED();
    if (layout != D3D12_BARRIER_LAYOUT_UNDEFINED || clearValue) throw std::invalid_argument("Placed buffers require undefined layout and no clear value");
    auto backing = CreateBufferBacking(self, address, *description);
    auto wrapper = std::make_unique<Resource>(std::move(backing), self, address, *description);
    *resource = wrapper.release();
    return S_OK;
}
