#include "Buffers.hpp"
#include "../../src/host/Com.hpp"
#include <anybox/PhysicalMemory.hpp>
#include <anybox/platform/WindowsError.hpp>
#include <xmem.hpp>
#include <vector>

namespace {
class PhysicalStorage {
public:
    PhysicalStorage() {
        auto count = frames.size();
        if (XMemAllocatePhysicalPages(0x280, &count, frames.data()) != 1 || count != frames.size()) throw std::runtime_error("Physical GPU test allocation failed");
        try { lease.reset(AnyBoxAcquirePhysicalPages(2, frames.data() + 1)); }
        catch (...) { XMemFreePhysicalPages(frames.size(), frames.data()); throw; }
    }
    ~PhysicalStorage() {
        lease.reset();
        XMemFreePhysicalPages(frames.size(), frames.data());
    }
    std::span<std::byte> GetMemory() const { return lease->GetContiguousMemory(); }

private:
    std::array<std::uint64_t, 3> frames{};
    AnyBox::Graphics::ComOwner<AnyBox::IPhysicalPageLease> lease;
};
}

void AnyBox::Graphics::Tests::CheckPhysicalHeap(Device* device) {
    constexpr GUID deviceId{0x81DADC15, 0x2BAD, 0x4392, {0x93, 0xC5, 0x10, 0x13, 0x45, 0xC4, 0xAA, 0x98}};
    constexpr GUID heapId{0x6B3B2502, 0x6E51, 0x45B3, {0x90, 0xEE, 0x98, 0x84, 0x26, 0x5E, 0x8D, 0xF3}};
    constexpr GUID resourceId{0x696442BE, 0xA72E, 0x4059, {0xBC, 0x79, 0x5B, 0x5C, 0x98, 0x04, 0x0F, 0xAD}};
    PhysicalStorage storage;
    auto memory = storage.GetMemory();
    Require(memory.size() == 2 * PhysicalPageSize, "Physical GPU memory is not contiguous");
    void* output = nullptr;
    Windows::CheckHResult(device->GetHost()->QueryInterface(deviceId, &output), "Physical GPU memory device interface");
    ComOwner<ID3D12Device3> native(static_cast<ID3D12Device3*>(output));
    MEMORY_BASIC_INFORMATION mapping{};
    if (!VirtualQuery(memory.data(), &mapping, sizeof(mapping))) Windows::ThrowLastError("VirtualQuery physical GPU heap");
    Windows::CheckHResult(native->OpenExistingHeapFromAddress(mapping.AllocationBase, heapId, &output), "Open physical GPU memory heap");
    ComOwner<ID3D12Heap> heap(static_cast<ID3D12Heap*>(output));
    auto offset = reinterpret_cast<std::uintptr_t>(memory.data()) - reinterpret_cast<std::uintptr_t>(mapping.AllocationBase);
    Require(offset == PhysicalPageSize && heap->GetDesc().SizeInBytes >= offset + memory.size(), "Physical GPU heap did not retain the underlying section and offset");
    D3D12_RESOURCE_DESC description{};
    description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    description.Width = memory.size();
    description.Height = 1;
    description.DepthOrArraySize = 1;
    description.MipLevels = 1;
    description.SampleDesc.Count = 1;
    description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    description.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
    Windows::CheckHResult(native->CreatePlacedResource(heap.get(), offset, &description, D3D12_RESOURCE_STATE_COMMON, nullptr, resourceId, &output), "Create physical GPU buffer");
    ComOwner<ID3D12Resource> resource(static_cast<ID3D12Resource*>(output));
    CheckSharedBufferTransfer(device, resource.get(), memory);
}
