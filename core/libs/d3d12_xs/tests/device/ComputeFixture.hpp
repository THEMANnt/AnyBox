#pragma once

#include "Tests.hpp"
#include "../../src/list/List.hpp"
#include "../../src/list/Compute.hpp"
#include <d3d12_xs.hpp>

namespace AnyBox::Graphics::Tests {
class ComputeFixture {
public:
    explicit ComputeFixture(Device* device) {
        std::array<D3D12_ROOT_PARAMETER, 5> parameters{};
        parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
        parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
        parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[3].Constants = {1, 0, 14};
        D3D12_DESCRIPTOR_RANGE range{D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 1, 0, 0};
        parameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameters[4].DescriptorTable = {1, &range};
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
        description.Version = D3D_ROOT_SIGNATURE_VERSION_1_0;
        description.Desc_1_0 = {static_cast<UINT>(parameters.size()), parameters.data(), 0, nullptr, static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0x03000000)};
        ID3DBlob* blob = nullptr;
        Require(D3D12SerializeVersionedRootSignature(&description, &blob, nullptr) == 0 && blob, "Compute root fixture serialization failed");
        void* output = nullptr;
        try { Method<decltype(&Device::CreateRootSignature)>(device, 16)(device, 0, blob->GetBufferPointer(), blob->GetBufferSize(), RootSignatureId, &output); }
        catch (...) { blob->Release(); throw; }
        blob->Release();
        root = static_cast<RootSignature*>(output);
        Method<decltype(&Device::CreateCommandAllocator)>(device, 9)(device, D3D12_COMMAND_LIST_TYPE_COMPUTE, AllocatorId, &output);
        allocator = static_cast<Allocator*>(output);
        Method<decltype(&Device::CreateCommandList)>(device, 12)(device, 0, D3D12_COMMAND_LIST_TYPE_COMPUTE, allocator, nullptr, XboxGraphicsListId, &output);
        list = static_cast<CommandList*>(output);
    }
    ~ComputeFixture() {
        Method<decltype(&CommandList::Release)>(list, 2)(list);
        Method<decltype(&Allocator::Release)>(allocator, 2)(allocator);
        Method<decltype(&RootSignature::Release)>(root, 2)(root);
    }
    CommandList* GetList() const noexcept { return list; }
    RootSignature* GetRoot() const noexcept { return root; }
    Allocator* GetAllocator() const noexcept { return allocator; }
    const Commands::RootState& GetArguments() const { return list->GetComputeState()->GetArguments(); }
    static std::uint32_t ReadHeader(CommandList* list, unsigned parameter) {
        std::uint32_t value;
        std::memcpy(&value, reinterpret_cast<std::byte*>(list) + 0x344 + parameter * 4, 4);
        return value;
    }
    static std::uint32_t* GetWrite(CommandList* list) {
        std::uint32_t* pointer;
        std::memcpy(&pointer, reinterpret_cast<std::byte*>(list) + 24, sizeof(pointer));
        return pointer;
    }
    static void SetWrite(CommandList* list, std::uint32_t* pointer) { std::memcpy(reinterpret_cast<std::byte*>(list) + 24, &pointer, sizeof(pointer)); }

private:
    CommandList* list;
    RootSignature* root;
    Allocator* allocator;
};
}
