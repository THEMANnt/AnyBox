#include "Tests.hpp"
#include "../../src/list/List.hpp"
#include "../../src/queue/Queue.hpp"
#include "../../src/fence/Fence.hpp"
#include <array>

namespace {
void CheckList(AnyBox::Graphics::Device* device, D3D12_COMMAND_LIST_TYPE type, std::uint32_t flags) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    auto createAllocator = Method<decltype(&Device::CreateCommandAllocator)>(device, 9);
    auto createList = Method<decltype(&Device::CreateCommandList)>(device, 12);
    auto interfaceId = static_cast<std::uint32_t>(type) == 0x10 ? DmaListId : type == D3D12_COMMAND_LIST_TYPE_COPY ? GraphicsListId : XboxGraphicsListId;
    void* output = nullptr;
    Require(createAllocator(device, type, AllocatorId, &output) == 0 && output, "List test allocator creation failed");
    auto* allocator = static_cast<Allocator*>(output);
    if (flags != 0) {
        XboxListDescription description{type, flags, 1, 0};
        Require(Method<decltype(&Device::CreateCommandListX)>(device, 73)(device, &description, allocator, nullptr, interfaceId, &output) == 0 && output, "Deferred Xbox command list creation failed");
    } else {
        Require(createList(device, 0, type, allocator, nullptr, interfaceId, &output) == 0 && output, "Xbox command list creation failed");
    }
    auto* list = static_cast<CommandList*>(output);
    Require(list->GetCreationFlags() == flags, "Xbox command list creation flags were not retained");
    auto close = Method<decltype(&CommandList::Close)>(list, 9);
    auto reset = Method<decltype(&CommandList::Reset)>(list, 10);
    auto query = Method<decltype(&CommandList::QueryInterface)>(list, 0);
    auto release = Method<decltype(&CommandList::Release)>(list, 2);
    Require(Method<decltype(&CommandList::GetType)>(list, 8)(list) == type, "Xbox list type changed across the ABI");
    for (const auto& id : {CommandListId, interfaceId, UnknownId, ObjectId, DeviceChildId}) {
        void* identity = nullptr;
        Require(query(list, id, &identity) == 0 && identity == list, "Command list COM identity changed");
        Require(release(static_cast<CommandList*>(identity)) == 1, "Command list query did not retain its reference");
    }
    auto unavailable = Method<void (*)(void*, ...)>(list, 127);
    Reject<std::runtime_error>([&] { unavailable(list); });
    Require(close(list) == 0, "Command list close failed");
    Reject<std::logic_error>([&] { close(list); });

    if (type != D3D12_COMMAND_LIST_TYPE_BUNDLE) {
        D3D12_COMMAND_QUEUE_DESC description{type, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
        Require(Method<decltype(&Device::CreateCommandQueue)>(device, 8)(device, &description, QueueId, &output) == 0 && output, "List test queue creation failed");
        auto* queue = static_cast<Queue*>(output);
        auto execute = Method<decltype(&Queue::ExecuteCommandLists)>(queue, 10);
        Require(Method<decltype(&Device::CreateFence)>(device, 36)(device, 0, D3D12_FENCE_FLAG_NONE, FenceId, &output) == 0 && output, "List test fence creation failed");
        auto* fence = static_cast<Fence*>(output);
        auto signal = Method<decltype(&Queue::Signal)>(queue, 14);
        auto complete = Method<decltype(&Fence::SetEventOnCompletion)>(fence, 9);
        std::array<void*, 20> batch;
        batch.fill(list);
        Reject<std::runtime_error>([&] { execute(queue, 2, batch.data()); });
        for (auto count : {1U, 20U}) {
            if (count > 1) {
                for (std::size_t index = 1; index < count; ++index) {
                    Require(createList(device, 0, type, allocator, nullptr, interfaceId, &output) == 0 && output, "Batch command list creation failed");
                    auto* additional = static_cast<CommandList*>(output);
                    Require(close(additional) == 0, "Batch command list close failed");
                    batch[index] = additional;
                }
            }
            execute(queue, count, batch.data());
            Require(signal(queue, fence, count) == 0 && complete(fence, count, nullptr) == 0, "Submitted command lists did not finish on the GPU");
            for (std::size_t index = 1; index < count; ++index) Require(release(static_cast<CommandList*>(batch[index])) == 0, "Batch command list was not released");
            Require(Method<decltype(&Allocator::Reset)>(allocator, 8)(allocator) == 0 && reset(list, allocator, nullptr) == 0, "Completed command list could not be reused");
            Require(list->GetCreationFlags() == flags, "Reset changed Xbox command list creation flags");
            Reject<std::logic_error>([&] { execute(queue, 1, batch.data()); });
            Require(close(list) == 0, "Reused command list close failed");
        }
        Reject<std::invalid_argument>([&] { execute(queue, 1, nullptr); });
        Method<decltype(&Fence::Release)>(fence, 2)(fence);
        Method<decltype(&Queue::Release)>(queue, 2)(queue);
    } else {
        Require(reset(list, allocator, nullptr) == 0 && close(list) == 0, "Bundle command list could not be reused");
    }
    Require(release(list) == 0, "Command list final reference was not released");
    Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Command list retained its allocator after destruction");
}
}

void AnyBox::Graphics::Tests::CheckLists(Device* device) {
    for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_BUNDLE, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_TYPE_COPY, static_cast<D3D12_COMMAND_LIST_TYPE>(0x10)}) CheckList(device, type, 0);
    for (auto flags : {2U, 4U, 6U}) {
        for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE}) CheckList(device, type, flags);
    }
    auto create = Method<decltype(&Device::CreateCommandListX)>(device, 73);
    XboxListDescription description{D3D12_COMMAND_LIST_TYPE_DIRECT, 0, 1, 0};
    void* output = nullptr;
    Require(Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, description.type, AllocatorId, &output) == 0 && output, "Xbox list test allocator creation failed");
    auto* allocator = static_cast<Allocator*>(output);
    Require(create(device, &description, allocator, nullptr, XboxGraphicsListId, &output) == 0 && output, "Xbox extended command list creation failed");
    auto* list = static_cast<CommandList*>(output);
    Require(Method<decltype(&CommandList::Close)>(list, 9)(list) == 0, "Xbox extended command list close failed");
    Require(Method<decltype(&CommandList::Release)>(list, 2)(list) == 0, "Xbox extended command list was not released");
    Require(Method<decltype(&Allocator::Release)>(allocator, 2)(allocator) == 0, "Xbox list allocator was not released");
    description.flags = 1;
    Reject<std::runtime_error>([&] { create(device, &description, nullptr, nullptr, XboxGraphicsListId, &output); });
    Require(!output, "Rejected Xbox recording flags retained a command list");
}
