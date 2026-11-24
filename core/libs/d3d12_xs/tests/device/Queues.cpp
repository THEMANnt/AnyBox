#include "Tests.hpp"
#include "../../src/queue/Queue.hpp"
#include "../../src/queue/Description.hpp"
#include <d3d12_xs.hpp>

namespace {
void CheckQueue(AnyBox::Graphics::Device* device, std::size_t slot, D3D12_COMMAND_LIST_TYPE type) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    auto create = Method<decltype(&Device::CreateCommandQueue)>(device, slot);
    D3D12_COMMAND_QUEUE_DESC description{type, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 1};
    void* output = nullptr;
    Require(create(device, &description, QueueId, &output) == 0 && output, "Xbox command queue creation failed");
    auto* queue = static_cast<Queue*>(output);
    auto query = Method<decltype(&Queue::QueryInterface)>(queue, 0);
    auto release = Method<decltype(&Queue::Release)>(queue, 2);
    auto getDevice = Method<decltype(&Queue::GetDevice)>(queue, 7);
    auto getDescription = Method<decltype(&Queue::GetDescription)>(queue, 18);
    auto restored = getDescription(queue);
    Require(restored.Type == description.Type && restored.Priority == description.Priority && restored.Flags == description.Flags && restored.NodeMask == description.NodeMask, "Command queue description changed across the Xbox ABI");
    for (const auto& id : {QueueId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
        void* identity = nullptr;
        Require(query(queue, id, &identity) == 0 && identity == queue, "Command queue COM identity changed");
        Require(release(static_cast<Queue*>(identity)) == 1, "Command queue interface did not own its reference");
    }
    void* parent = nullptr;
    Require(getDevice(queue, XboxDeviceId, &parent) == 0 && parent == device, "Command queue returned the wrong parent device");
    auto releaseDevice = Method<decltype(&Device::Release)>(device, 2);
    Require(releaseDevice(static_cast<Device*>(parent)) == 2, "Command queue did not retain its parent device");
    auto frequency = Method<decltype(&Queue::GetTimestampFrequency)>(queue, 16);
    std::uint64_t ticks = 0;
    Require(frequency(queue, &ticks) == 0 && ticks != 0, "Command queue timestamp frequency is invalid");
    Reject<std::invalid_argument>([&] { frequency(queue, nullptr); });
    auto name = Method<decltype(&Queue::SetName)>(queue, 6);
    Require(name(queue, L"AnyBox command queue ABI test") == 0, "Command queue naming failed");
    auto unavailable = Method<void (*)(void*, ...)>(queue, 127);
    Reject<std::runtime_error>([&] { unavailable(queue); });
    Require(release(queue) == 0, "Command queue final reference was not released");
    Reject<std::invalid_argument>([&] { create(device, &description, QueueId, nullptr); });
}
}

void AnyBox::Graphics::Tests::CheckQueues(Device* device) {
    for (auto slot : {std::size_t{8}}) {
        for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_TYPE_COPY, static_cast<D3D12_COMMAND_LIST_TYPE>(0x10)}) {
            auto create = Method<decltype(&Device::CreateCommandQueue)>(device, slot);
            Reject<std::invalid_argument>([&] { create(device, nullptr, QueueId, nullptr); });
            CheckQueue(device, slot, type);
        }
    }
    auto create = Method<decltype(&Device::CreateCommandQueueX)>(device, 48);
    for (auto engine : {0U, 1U, 2U, 3U}) {
        XboxQueueDescription description{2, 0, engine, engine == 0 ? 1U : 0U};
        void* output = nullptr;
        Require(create(device, &description, QueueId, &output) == 0 && output, "Xbox compute engine queue creation failed");
        auto* queue = static_cast<Queue*>(output);
        void* duplicate = nullptr;
        Reject<std::logic_error>([&] { create(device, &description, QueueId, &duplicate); });
        Require(!duplicate, "Duplicate Xbox hardware queue retained an output");
        auto restored = Method<decltype(&Queue::GetDescription)>(queue, 18)(queue);
        Require(restored.Type == D3D12_COMMAND_LIST_TYPE_COMPUTE && restored.Flags == D3D12_COMMAND_QUEUE_FLAG_NONE && restored.NodeMask == 0, "Xbox engine selector was mistaken for desktop queue flags");
        Require(Method<decltype(&Queue::Release)>(queue, 2)(queue) == 0, "Xbox compute queue was not released");
        description.engineIndex = 4;
        Reject<std::invalid_argument>([&] { create(device, &description, QueueId, &output); });
    }
}
