#include "TextureMemory.hpp"
#include <xmem.hpp>
#include <bit>
#include <limits>
#include <utility>

AnyBox::Graphics::TextureMemory::TextureMemory(std::uint64_t size, std::uint64_t alignment, D3D12_HEAP_FLAGS flags) : size(size), attributes(0) {
    if (size == 0 || size > std::numeric_limits<std::size_t>::max() || !std::has_single_bit(alignment) || alignment < 4096 || alignment > 2097152) throw std::invalid_argument("Texture allocation size or alignment is invalid");
    if ((static_cast<std::uint32_t>(flags) & ~static_cast<std::uint32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY)) != 0) throw std::runtime_error("Texture allocation heap flags are not implemented");
    bool display = (flags & D3D12_HEAP_FLAG_ALLOW_DISPLAY) != 0;
    auto pageMode = display || alignment > 65536 ? 2ULL : 1ULL;
    auto exponent = static_cast<std::uint64_t>(std::countr_zero(alignment));
    attributes = (1ULL << 33) | (1ULL << 29) | (exponent << 24) | (pageMode << 14) | (display ? 0x800013 : 0x800004);
    address = XMemAlloc(static_cast<std::size_t>(size), attributes);
    if (!address) throw std::bad_alloc();
    if (reinterpret_cast<std::uintptr_t>(address) % alignment != 0 || reinterpret_cast<std::uintptr_t>(address) % 65536 != 0) {
        XMemFree(address, attributes);
        throw std::invalid_argument("Texture allocation hook returned a misaligned address");
    }
}

AnyBox::Graphics::TextureMemory::TextureMemory(TextureMemory&& other) noexcept : address(std::exchange(other.address, nullptr)), size(other.size), attributes(other.attributes) {}

AnyBox::Graphics::TextureMemory::~TextureMemory() {
    if (address) XMemFree(address, attributes);
}
