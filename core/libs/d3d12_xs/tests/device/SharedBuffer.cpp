#include "Buffers.hpp"
#include "../../src/host/Com.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <vector>

void AnyBox::Graphics::Tests::CheckSharedBufferTransfer(Device* device, ID3D12Resource* resource, std::span<std::byte> memory) {
    constexpr GUID allocatorId{0x6102DEE4, 0xAF59, 0x4B09, {0xB9, 0x99, 0xB4, 0x4D, 0x73, 0xF0, 0x9B, 0x24}};
    constexpr GUID listId{0x5B160D0F, 0xAC1B, 0x4185, {0x8B, 0xA8, 0xB3, 0xAE, 0x42, 0xA5, 0xA4, 0x55}};
    constexpr GUID queueId{0x0EC870A6, 0x5D7E, 0x4C22, {0x8C, 0xFC, 0x5B, 0xAA, 0xE0, 0x76, 0x16, 0xED}};
    constexpr GUID fenceId{0x0A753DCF, 0xC4D8, 0x4B91, {0xAD, 0xF6, 0xBE, 0x5A, 0x60, 0xD9, 0x5A, 0x76}};
    std::vector<std::byte> expected(TransferSize);
    for (std::size_t index = 0; index < expected.size(); ++index) expected[index] = static_cast<std::byte>((index * 71) ^ (index >> 9));
    std::memcpy(memory.data(), expected.data(), expected.size());
    void* output = nullptr;
    auto buffers = CreateNativeBuffers(device);
    Windows::CheckHResult(device->GetHost()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, allocatorId, &output), "Physical GPU memory allocator");
    ComOwner<ID3D12CommandAllocator> allocator(static_cast<ID3D12CommandAllocator*>(output));
    Windows::CheckHResult(device->GetHost()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.get(), nullptr, listId, &output), "Physical GPU memory command list");
    ComOwner<ID3D12GraphicsCommandList> list(static_cast<ID3D12GraphicsCommandList*>(output));
    list->CopyBufferRegion(buffers.readback.get(), 0, resource, 0, TransferSize);
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition = {resource, D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST};
    list->ResourceBarrier(1, &barrier);
    list->CopyBufferRegion(resource, 0, buffers.upload.get(), 0, TransferSize);
    Windows::CheckHResult(list->Close(), "Close physical GPU memory command list");
    D3D12_COMMAND_QUEUE_DESC queueDescription{D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    Windows::CheckHResult(device->GetHost()->CreateCommandQueue(&queueDescription, queueId, &output), "Physical GPU memory queue");
    ComOwner<ID3D12CommandQueue> queue(static_cast<ID3D12CommandQueue*>(output));
    Windows::CheckHResult(device->GetHost()->CreateFence(0, D3D12_FENCE_FLAG_NONE, fenceId, &output), "Physical GPU memory fence");
    ComOwner<ID3D12Fence> fence(static_cast<ID3D12Fence*>(output));
    ID3D12CommandList* commands[]{list.get()};
    queue->ExecuteCommandLists(1, commands);
    Windows::CheckHResult(queue->Signal(fence.get(), 1), "Signal physical GPU memory completion");
    Windows::CheckHResult(fence->SetEventOnCompletion(1, nullptr), "Wait physical GPU memory completion");
    Windows::CheckHResult(device->GetHost()->GetDeviceRemovedReason(), "Physical GPU memory device status");
    void* mapped = nullptr;
    D3D12_RANGE range{0, TransferSize};
    Windows::CheckHResult(buffers.readback->Map(0, &range, &mapped), "Read physical GPU memory transfer");
    Require(std::memcmp(mapped, expected.data(), expected.size()) == 0, "GPU did not read the actual XMem physical pages");
    range = {0, 0};
    buffers.readback->Unmap(0, &range);
    Require(memory[18] == static_cast<std::byte>(18 * 37), "GPU writes did not reach the canonical XMem physical pages");
    VerifyReadback(resource);
}
