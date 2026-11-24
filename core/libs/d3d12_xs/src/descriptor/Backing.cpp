#include "Backing.hpp"
#include <xmem.hpp>
#include <limits>
#include <utility>

AnyBox::Graphics::DescriptorBacking::DescriptorBacking(DescriptorBacking&& other) noexcept : address(std::exchange(other.address, nullptr)), size(other.size), attributes(other.attributes), buffer(std::move(other.buffer)) {}

AnyBox::Graphics::DescriptorBacking::~DescriptorBacking() {
    buffer.reset();
    if (address) XMemFree(address, attributes);
}

AnyBox::Graphics::DescriptorBacking AnyBox::Graphics::CreateDescriptorBacking(Device* device, const D3D12_DESCRIPTOR_HEAP_DESC& description) {
    auto stride = Device::GetDescriptorHandleIncrementSize(device, description.Type);
    auto bytes = static_cast<std::uint64_t>(stride) * description.NumDescriptors;
    if (description.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) bytes += (bytes + 271) & ~std::uint64_t{255};
    if (bytes > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("Xbox descriptor heap byte size exceeds 32 bits");
    DescriptorBacking backing;
    if (bytes == 0) return backing;
    backing.size = static_cast<std::size_t>((bytes + 65535) & ~std::uint64_t{65535});
    auto kind = (description.Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE) != 0 ? 1ULL : 3ULL;
    backing.attributes = (std::uint64_t{1} << 33) | (kind << 29) | (16ULL << 24) | (1ULL << 14) | (kind == 1 ? 0x800004 : 0x800006);
    backing.address = XMemAlloc(backing.size, backing.attributes);
    if (!backing.address) throw std::bad_alloc();
    if (reinterpret_cast<std::uintptr_t>(backing.address) % 65536 != 0) throw std::invalid_argument("Xbox descriptor allocation hook returned a misaligned address");
    if (description.Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER) std::memcpy(backing.address, &description.NumDescriptors, sizeof(description.NumDescriptors));

    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    std::span memory(static_cast<std::byte*>(backing.address), backing.size);
    auto buffer = Vulkan::Buffer::Import(device->GetGpuContext(), memory, bytes, usage);
    backing.buffer = std::make_unique<Vulkan::Buffer>(std::move(buffer));
    return backing;
}
