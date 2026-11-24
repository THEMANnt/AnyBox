#include "Tests.hpp"
#include "../../src/fence/Fence.hpp"
#include "../../src/queue/Queue.hpp"
#include <memory>

namespace {
struct HandleDeleter {
    void operator()(void* value) const noexcept { CloseHandle(value); }
};
}

void AnyBox::Graphics::Tests::CheckFences(Device* device) {
    auto create = Method<decltype(&Device::CreateFence)>(device, 36);
    void* output = nullptr;
    Require(create(device, 3, D3D12_FENCE_FLAG_SHARED, FenceId, &output) == 0 && output, "Shared fence creation failed");
    auto* fence = static_cast<Fence*>(output);
    auto release = Method<decltype(&Fence::Release)>(fence, 2);
    auto query = Method<decltype(&Fence::QueryInterface)>(fence, 0);
    auto completed = Method<decltype(&Fence::GetCompletedValue)>(fence, 8);
    auto setEvent = Method<decltype(&Fence::SetEventOnCompletion)>(fence, 9);
    auto signalCpu = Method<decltype(&Fence::Signal)>(fence, 10);
    auto flags = Method<decltype(&Fence::GetCreationFlags)>(fence, 11);
    Require(completed(fence) == 3 && flags(fence) == D3D12_FENCE_FLAG_SHARED, "Fence initial state changed across the Xbox ABI");
    for (const auto& id : {FenceId, Fence1Id, UnknownId, ObjectId, DeviceChildId, PageableId}) {
        void* identity = nullptr;
        Require(query(fence, id, &identity) == 0 && identity == fence, "Fence COM identity changed");
        Require(release(static_cast<Fence*>(identity)) == 1, "Fence interface did not own its reference");
    }
    auto getDevice = Method<decltype(&Fence::GetDevice)>(fence, 7);
    void* parent = nullptr;
    Require(getDevice(fence, XboxDeviceId, &parent) == 0 && parent == device, "Fence parent identity changed");
    Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(parent));
    Reject<std::invalid_argument>([&] { create(device, 0, D3D12_FENCE_FLAG_NONE, FenceId, nullptr); });
    Guid unknown{};
    Reject<std::invalid_argument>([&] { query(fence, unknown, &output); });
    Require(!output, "Rejected fence query retained its output");

    D3D12_COMMAND_QUEUE_DESC description{D3D12_COMMAND_LIST_TYPE_DIRECT, 0, D3D12_COMMAND_QUEUE_FLAG_NONE, 1};
    auto createQueue = Method<decltype(&Device::CreateCommandQueue)>(device, 8);
    Require(createQueue(device, &description, QueueId, &output) == 0 && output, "Fence test queue creation failed");
    auto* queue = static_cast<Queue*>(output);
    auto signalGpu = Method<decltype(&Queue::Signal)>(queue, 14);
    auto waitGpu = Method<decltype(&Queue::Wait)>(queue, 15);
    std::unique_ptr<void, HandleDeleter> event(CreateEventW(nullptr, FALSE, FALSE, nullptr));
    Require(event != nullptr, "Fence completion event creation failed");
    Require(setEvent(fence, 5, event.get()) == 0 && signalGpu(queue, fence, 5) == 0, "GPU fence signal failed");
    Require(WaitForSingleObject(event.get(), 5000) == WAIT_OBJECT_0 && completed(fence) >= 5, "GPU fence completion was not delivered");
    Require(waitGpu(queue, fence, 9) == 0 && signalGpu(queue, fence, 10) == 0, "GPU queue wait failed");
    Require(setEvent(fence, 10, event.get()) == 0, "Delayed GPU fence event registration failed");
    auto premature = WaitForSingleObject(event.get(), 20);
    Require(signalCpu(fence, 9) == 0, "CPU fence signal failed");
    Require(WaitForSingleObject(event.get(), 5000) == WAIT_OBJECT_0 && completed(fence) >= 10, "CPU signal did not release the queued GPU wait");
    Require(premature == WAIT_TIMEOUT, "GPU signal bypassed its preceding wait");
    Require(setEvent(fence, 10, nullptr) == 0, "Completed synchronous fence wait failed");
    Reject<std::invalid_argument>([&] { signalGpu(queue, nullptr, 0); });
    Require(Method<decltype(&Queue::Release)>(queue, 2)(queue) == 0, "Fence test queue was not released");
    Require(release(fence) == 0, "Fence final reference was not released");
}
