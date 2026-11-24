#include "Device.hpp"
#include "../descriptor/Heap.hpp"

HRESULT AnyBox::Graphics::Device::CreateDescriptorHeap(Device* self, const D3D12_DESCRIPTOR_HEAP_DESC* description, const Guid& interfaceId, void** heap) {
    if (!heap) throw std::invalid_argument("Descriptor heap output is null");
    *heap = nullptr;
    if (!description || interfaceId != DescriptorHeapId) throw std::invalid_argument("Descriptor heap description or interface is invalid");
    if (description->NodeMask > 1 || (static_cast<std::uint32_t>(description->Flags) & ~1u) != 0) throw std::runtime_error("Descriptor heap node or flag mode is not implemented");
    auto backing = CreateDescriptorBacking(self, *description);
    auto wrapper = std::make_unique<DescriptorHeap>(std::move(backing), self, *description);
    *heap = wrapper.release();
    return S_OK;
}
