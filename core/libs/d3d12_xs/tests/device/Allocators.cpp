#include "Tests.hpp"
#include "../../src/allocator/Allocator.hpp"

void AnyBox::Graphics::Tests::CheckAllocators(Device* device) {
    auto create = Method<decltype(&Device::CreateCommandAllocator)>(device, 9);
    for (auto type : {D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_TYPE_BUNDLE, D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_LIST_TYPE_COPY, static_cast<D3D12_COMMAND_LIST_TYPE>(0x10)}) {
        void* output = nullptr;
        Require(create(device, type, AllocatorId, &output) == 0 && output, "Command allocator creation failed");
        auto* allocator = static_cast<Allocator*>(output);
        auto release = Method<decltype(&Allocator::Release)>(allocator, 2);
        auto query = Method<decltype(&Allocator::QueryInterface)>(allocator, 0);
        auto reset = Method<decltype(&Allocator::Reset)>(allocator, 8);
        for (const auto& id : {AllocatorId, UnknownId, ObjectId, DeviceChildId, PageableId}) {
            void* identity = nullptr;
            Require(query(allocator, id, &identity) == 0 && identity == allocator, "Allocator COM identity changed");
            Require(release(static_cast<Allocator*>(identity)) == 1, "Allocator interface did not own a reference");
        }
        void* parent = nullptr;
        Require(Method<decltype(&Allocator::GetDevice)>(allocator, 7)(allocator, XboxDeviceId, &parent) == 0 && parent == device, "Allocator returned a foreign parent device");
        Method<decltype(&Device::Release)>(device, 2)(static_cast<Device*>(parent));
        Require(reset(allocator) == 0 && reset(allocator) == 0, "Unused allocator could not be reset");
        Require(Method<decltype(&Allocator::SetName)>(allocator, 6)(allocator, L"AnyBox allocator ABI test") == 0, "Allocator naming failed");
        Require(release(allocator) == 0, "Allocator final reference was not released");
    }
    void* output = nullptr;
    Reject<std::invalid_argument>([&] { create(device, D3D12_COMMAND_LIST_TYPE_DIRECT, AllocatorId, nullptr); });
    Guid unknown{};
    Reject<std::invalid_argument>([&] { create(device, D3D12_COMMAND_LIST_TYPE_DIRECT, unknown, &output); });
    Require(!output, "Rejected allocator creation retained an output");
}
