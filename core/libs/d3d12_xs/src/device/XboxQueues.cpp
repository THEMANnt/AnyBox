#include "Device.hpp"
#include "../queue/Description.hpp"
#include "../queue/Queue.hpp"
#include <array>
#include <stdexcept>

HRESULT AnyBox::Graphics::Device::CreateCommandQueueX(Device* self, const XboxQueueDescription* description, const Guid& interfaceId, void** queue) {
    if (!queue) throw std::invalid_argument("Xbox command queue output is null");
    *queue = nullptr;
    if (!description) throw std::invalid_argument("Xbox command queue description is null");
    if (description->flags != 0) throw std::runtime_error("Xbox command queue flags are not implemented");
    if (description->type != static_cast<std::uint32_t>(D3D12_COMMAND_LIST_TYPE_COMPUTE)) throw std::runtime_error("Xbox command queue engine selection is not implemented for this type");
    constexpr std::array<std::uint32_t, 4> queueCounts{6, 3, 4, 4};
    constexpr std::array<std::uint32_t, 4> queueOffsets{0, 6, 9, 13};
    if (description->engineIndex >= queueCounts.size() || description->queueIndex >= queueCounts[description->engineIndex]) throw std::invalid_argument("Xbox compute queue selection is out of range");

    D3D12_COMMAND_QUEUE_DESC nativeDescription{D3D12_COMMAND_LIST_TYPE_COMPUTE, D3D12_COMMAND_QUEUE_PRIORITY_NORMAL, D3D12_COMMAND_QUEUE_FLAG_NONE, 0};
    auto selection = std::uint32_t{1} << (queueOffsets[description->engineIndex] + description->queueIndex);
    auto previous = self->computeQueues.fetch_or(selection, std::memory_order_acq_rel);
    if (previous & selection) throw std::logic_error("Xbox compute queue selection is already in use");
    try {
        auto result = CreateCommandQueue(self, &nativeDescription, interfaceId, queue);
        static_cast<Queue*>(*queue)->SetXboxSelection(selection);
        return result;
    } catch (...) {
        self->ReleaseQueueSelection(selection);
        throw;
    }
}

void AnyBox::Graphics::Device::ReleaseQueueSelection(std::uint32_t selection) noexcept { computeQueues.fetch_and(~selection, std::memory_order_release); }
