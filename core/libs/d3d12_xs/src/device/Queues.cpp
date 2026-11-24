#include "Device.hpp"
#include "../queue/Queue.hpp"
#include <memory>
#include <stdexcept>

namespace {
struct QueueDeleter {
    void operator()(ID3D12CommandQueue* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateCommandQueue(Device* self, const D3D12_COMMAND_QUEUE_DESC* description, const Guid& interfaceId, void** queue) {
    if (!queue) throw std::invalid_argument("Command queue output is null");
    *queue = nullptr;
    if (!description) throw std::invalid_argument("Command queue description is null");
    if (!Queue::Supports(interfaceId)) throw std::invalid_argument("Unsupported command queue creation interface");
    auto nativeDescription = *description;
    if (static_cast<std::uint32_t>(nativeDescription.Type) == 0x10) nativeDescription.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    void* host = nullptr;
    auto result = self->host->CreateCommandQueue(&nativeDescription, NativeGuid(QueueId), &host);
    CheckGraphicsResult(result, "ID3D12Device::CreateCommandQueue");
    if (!host) throw std::runtime_error("D3D12 command queue creation returned no interface");
    std::unique_ptr<ID3D12CommandQueue, QueueDeleter> ownedHost(static_cast<ID3D12CommandQueue*>(host));
    auto wrapper = std::make_unique<Queue>(ownedHost.get(), self, *description);
    ownedHost.release();
    *queue = wrapper.release();
    return result;
}
