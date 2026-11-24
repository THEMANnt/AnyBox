#include "Device.hpp"
#include "../resource/TextureAllocation.hpp"
#include <algorithm>
#include <bit>
#include <limits>
#include <stdexcept>

D3D12_RESOURCE_ALLOCATION_INFO AnyBox::Graphics::Device::GetResourceAllocationInfo(Device* self, std::uint32_t nodeMask, std::uint32_t count, const D3D12_RESOURCE_DESC* descriptions) {
    if (!self || (count != 0 && !descriptions)) throw std::invalid_argument("Resource allocation query arguments are null");
    static_cast<void>(nodeMask);
    D3D12_RESOURCE_ALLOCATION_INFO result{0, 1};
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto& description = descriptions[index];
        D3D12_RESOURCE_ALLOCATION_INFO allocation;
        if (description.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
            allocation = {description.Width, std::max<std::uint64_t>(4, description.Alignment)};
        } else {
            allocation = ComputeTextureAllocation(description);
        }
        if (!std::has_single_bit(allocation.Alignment)) throw std::invalid_argument("Resource allocation alignment is not a power of two");
        auto padding = (0 - result.SizeInBytes) & (allocation.Alignment - 1);
        constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
        if (result.SizeInBytes > maximum - padding || allocation.SizeInBytes > maximum - result.SizeInBytes - padding) throw std::overflow_error("Resource allocation query size overflows");
        result.SizeInBytes += padding + allocation.SizeInBytes;
        result.Alignment = std::max(result.Alignment, allocation.Alignment);
    }
    return result;
}
