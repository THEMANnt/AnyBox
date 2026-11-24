#include "Device.hpp"
#include "../fence/Fence.hpp"
#include <memory>
#include <stdexcept>

namespace {
struct FenceDeleter {
    void operator()(ID3D12Fence* value) const noexcept { value->Release(); }
};
}

HRESULT AnyBox::Graphics::Device::CreateFence(Device* self, std::uint64_t initialValue, D3D12_FENCE_FLAGS flags, const Guid& interfaceId, void** fence) {
    if (!fence) throw std::invalid_argument("Fence output is null");
    *fence = nullptr;
    if (!Fence::Supports(interfaceId)) throw std::invalid_argument("Unsupported fence creation interface");
    void* host = nullptr;
    auto result = self->host->CreateFence(initialValue, flags, NativeGuid(FenceId), &host);
    CheckGraphicsResult(result, "ID3D12Device::CreateFence");
    if (!host) throw std::runtime_error("D3D12 fence creation returned no interface");
    std::unique_ptr<ID3D12Fence, FenceDeleter> ownedHost(static_cast<ID3D12Fence*>(host));
    auto wrapper = std::make_unique<Fence>(ownedHost.get(), self, flags);
    ownedHost.release();
    *fence = wrapper.release();
    return result;
}
