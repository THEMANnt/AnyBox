#include "Heap.hpp"
#include <xmem.hpp>
#include <bit>
#include <utility>

AnyBox::Graphics::DescriptorHeap::DescriptorHeap(DescriptorBacking backing, Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description) : vtable(&GetDescriptorHeapTable()), type(description.Type), count(description.NumDescriptors), strideShift(std::bit_width(Device::GetDescriptorHandleIncrementSize(device, type)) - 1), address(reinterpret_cast<std::uintptr_t>(backing.address)), flags(description.Flags), allocation(backing.address), allocationSize(backing.size), attributes(backing.attributes), addressMirror(address), device(device), buffer(backing.buffer.get()) {
    Device::AddRef(device);
    backing.buffer.release();
    backing.address = nullptr;
}

AnyBox::Graphics::DescriptorHeap::~DescriptorHeap() {
    privateData.Reset();
    delete buffer;
    if (allocation) XMemFree(allocation, attributes);
    Device::Release(device);
}

bool AnyBox::Graphics::DescriptorHeap::Supports(const Guid& id) noexcept { return id == DescriptorHeapId || id == UnknownId || id == ObjectId || id == DeviceChildId || id == PageableId; }
