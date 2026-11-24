#include "../device/Device.hpp"
#include <cstddef>

static_assert(sizeof(D3D12_CONSTANT_BUFFER_VIEW_DESC) == 16);
static_assert(offsetof(D3D12_CONSTANT_BUFFER_VIEW_DESC, BufferLocation) == 0);
static_assert(offsetof(D3D12_CONSTANT_BUFFER_VIEW_DESC, SizeInBytes) == 8);

void AnyBox::Graphics::Device::CreateConstantBufferView(Device* self, const D3D12_CONSTANT_BUFFER_VIEW_DESC* description, std::uint64_t destination) {
    if (destination == 0 || destination % 16 != 0) throw std::invalid_argument("Xbox constant view destination is null or misaligned");
    auto address = description ? description->BufferLocation : self->nullConstantAddress;
    auto count = description ? (static_cast<std::uint64_t>(description->SizeInBytes) + 15) >> 4 : 1;
    const std::array<std::uint64_t, 2> words{address + 0x10000000000000ULL, count + 0x2201403C00000000ULL};
    std::memcpy(reinterpret_cast<void*>(static_cast<std::uintptr_t>(destination)), words.data(), sizeof(words));
}
