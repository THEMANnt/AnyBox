#include "Buffers.hpp"
#include "../../src/list/List.hpp"
#include "../../src/queue/Queue.hpp"
#include "../../src/fence/Fence.hpp"

namespace {
void CheckPipeline(AnyBox::Graphics::Device* device, D3D12_COMMAND_LIST_TYPE type) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, type, AllocatorId, &output) == 0 && output, "Synchronization allocator creation failed");
    auto* allocator = static_cast<Allocator*>(output);
    XboxListDescription description{type, 2, 1, 0};
    Require(Method<decltype(&Device::CreateCommandListX)>(device, 73)(device, &description, allocator, nullptr, XboxGraphicsListId, &output) == 0 && output, "Synchronization list creation failed");
    auto* list = static_cast<CommandList*>(output);
    auto buffers = CreateNativeBuffers(device);
    auto top = Method<decltype(&CommandList::SyncTopOfPipeX)>(list, 131);
    auto bottom = Method<decltype(&CommandList::SyncBottomOfPipeX)>(list, 132);
    constexpr auto fullRange = (std::uint64_t{1} << 48) - 256;
    auto topFlags = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? 0xC3C0U : 0xC380U;
    auto topControl = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? 0x80020000U : 0x20000U;
    auto bottomFlags = type == D3D12_COMMAND_LIST_TYPE_DIRECT ? 0x700000U : 0x100000U;
    Reject<std::invalid_argument>([&] { top(list, 0, 0, 0, fullRange); });
    Reject<std::invalid_argument>([&] { top(list, topFlags, 0x30000, 0, fullRange); });
    Reject<std::invalid_argument>([&] { top(list, topFlags, topControl, 1, fullRange); });
    Reject<std::runtime_error>([&] { top(list, topFlags, topControl, 0, 65536); });
    Reject<std::runtime_error>([&] { top(list, topFlags | 0x40000000, topControl, 0, fullRange); });

    list->GetHost()->CopyBufferRegion(buffers.device.get(), 0, buffers.upload.get(), 0, TransferSize);
    bottom(list, bottomFlags, 0, 0, fullRange);
    top(list, topFlags, topControl, 0, fullRange);
    list->GetHost()->CopyBufferRegion(buffers.readback.get(), 0, buffers.device.get(), 0, TransferSize);
    Require(Method<decltype(&CommandList::Close)>(list, 9)(list) == 0, "Synchronized command list close failed");
    Reject<std::logic_error>([&] { top(list, topFlags, topControl, 0, fullRange); });

    D3D12_COMMAND_QUEUE_DESC queueDescription{type, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    Require(Method<decltype(&Device::CreateCommandQueue)>(device, 8)(device, &queueDescription, QueueId, &output) == 0 && output, "Synchronization queue creation failed");
    auto* queue = static_cast<Queue*>(output);
    Require(Method<decltype(&Device::CreateFence)>(device, 36)(device, 0, D3D12_FENCE_FLAG_NONE, FenceId, &output) == 0 && output, "Synchronization fence creation failed");
    auto* fence = static_cast<Fence*>(output);
    void* commands[]{list};
    Method<decltype(&Queue::ExecuteCommandLists)>(queue, 10)(queue, 1, commands);
    Require(Method<decltype(&Queue::Signal)>(queue, 14)(queue, fence, 1) == 0, "Synchronization completion signal failed");
    Require(Method<decltype(&Fence::SetEventOnCompletion)>(fence, 9)(fence, 1, nullptr) == 0, "Synchronization GPU work did not finish");
    Require(Method<decltype(&Device::GetDeviceRemovedReason)>(device, 37)(device) == 0, "Synchronized GPU work removed the device");
    VerifyReadback(buffers.readback.get());
    Method<decltype(&Fence::Release)>(fence, 2)(fence);
    Method<decltype(&Queue::Release)>(queue, 2)(queue);
    Require(Method<decltype(&CommandList::Release)>(list, 2)(list) == 0, "Synchronization list was not released");
    Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Synchronization allocator was not released");
}
}

void AnyBox::Graphics::Tests::CheckSynchronization(Device* device) {
    for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE}) CheckPipeline(device, type);
}
