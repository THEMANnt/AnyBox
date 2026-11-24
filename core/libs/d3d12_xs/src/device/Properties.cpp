#include "Device.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <system_error>

GUID AnyBox::Graphics::NativeGuid(const Guid& value) {
    GUID result{value.data1, value.data2, value.data3, {}};
    std::copy(value.data4.begin(), value.data4.end(), result.Data4);
    return result;
}

void AnyBox::Graphics::CheckGraphicsResult(HRESULT result, const char* operation) {
    if (FAILED(result)) throw std::system_error(static_cast<int>(result), std::system_category(), operation);
}

std::uint32_t AnyBox::Graphics::Device::GetNodeCount(Device* self) { return self->host->GetNodeCount(); }

std::uint32_t AnyBox::Graphics::Device::GetDescriptorHandleIncrementSize(Device*, D3D12_DESCRIPTOR_HEAP_TYPE type) {
    constexpr std::array<std::uint32_t, 5> sizes{32, 16, 112, 128, 16};
    auto index = static_cast<std::uint32_t>(type);
    if (index >= sizes.size()) throw std::invalid_argument("Invalid graphics descriptor heap type");
    return sizes[index];
}

D3D12_HEAP_PROPERTIES AnyBox::Graphics::Device::GetCustomHeapProperties(Device* self, std::uint32_t nodeMask, D3D12_HEAP_TYPE type) {
    if (type < D3D12_HEAP_TYPE_DEFAULT || type > D3D12_HEAP_TYPE_READBACK) throw std::invalid_argument("Invalid graphics heap type");
    auto count = self->host->GetNodeCount();
    if (count == 0 || count > 32 || (count > 1 && nodeMask == 0)) throw std::invalid_argument("Invalid graphics heap node mask");
    if (nodeMask != 0 && (!std::has_single_bit(nodeMask) || (count < 32 && nodeMask >= (1u << count)))) throw std::invalid_argument("Invalid graphics heap node mask");
    return self->host->GetCustomHeapProperties(nodeMask, type);
}
