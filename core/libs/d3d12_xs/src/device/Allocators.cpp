#include "Device.hpp"
#include "../allocator/Allocator.hpp"
#include <memory>
#include <stdexcept>

namespace {
struct AllocatorDeleter {
    void operator()(ID3D12CommandAllocator* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateCommandAllocator(Device* self, D3D12_COMMAND_LIST_TYPE type, const Guid& interfaceId, void** allocator) {
    if (!allocator) throw std::invalid_argument("Command allocator output is null");
    *allocator = nullptr;
    if (!Allocator::Supports(interfaceId)) throw std::invalid_argument("Unsupported command allocator creation interface");
    auto nativeType = static_cast<std::uint32_t>(type) == 0x10 ? D3D12_COMMAND_LIST_TYPE_COPY : type;
    void* host = nullptr;
    auto result = self->host->CreateCommandAllocator(nativeType, NativeGuid(AllocatorId), &host);
    CheckGraphicsResult(result, "ID3D12Device::CreateCommandAllocator");
    if (!host) throw std::runtime_error("D3D12 command allocator creation returned no interface");
    std::unique_ptr<ID3D12CommandAllocator, AllocatorDeleter> ownedHost(static_cast<ID3D12CommandAllocator*>(host));
    auto wrapper = std::make_unique<Allocator>(ownedHost.get(), self, type);
    ownedHost.release();
    *allocator = wrapper.release();
    return result;
}
