#include "TextureAllocation.hpp"
#include <anybox/address/Context.hpp>
#include <bit>
#include <stdexcept>

namespace {
std::uint32_t BitsPerPixel(DXGI_FORMAT format) {
    auto value = static_cast<std::uint32_t>(format);
    if (value >= 1 && value <= 4) return 128;
    if (value >= 9 && value <= 18) return 64;
    if ((value >= 23 && value <= 43) || value == 67 || (value >= 87 && value <= 93)) return 32;
    if ((value >= 48 && value <= 59) || value == 85 || value == 86) return 16;
    if (value >= 60 && value <= 65) return 8;
    throw std::runtime_error("Texture allocation format is not implemented");
}

std::uint32_t SwizzleMode(const D3D12_RESOURCE_DESC& description, std::uint32_t bits) {
    auto layout = static_cast<std::uint32_t>(description.Layout);
    if (layout == 1) return 0;
    if (layout >= 256 && layout <= 287) {
        auto mode = layout - 256;
        if (mode == 0 || mode == 5 || mode == 21 || mode == 27) return mode;
    }
    if (layout != 0) throw std::runtime_error("Texture allocation layout is not implemented");
    auto flags = static_cast<std::uint32_t>(description.Flags);
    if ((flags & 1) != 0) return 27;
    if ((flags & 4) == 0) return 5;
    return description.Width * description.Height * bits <= 0x80000 ? 21 : 27;
}
}

std::uint32_t AnyBox::Graphics::ComputeTextureSwizzleMode(const D3D12_RESOURCE_DESC& description) {
    return SwizzleMode(description, BitsPerPixel(description.Format));
}

D3D12_RESOURCE_ALLOCATION_INFO AnyBox::Graphics::ComputeTextureAllocation(const D3D12_RESOURCE_DESC& description) {
    auto surface = ComputeTextureSurface(description);
    return {surface.size, surface.alignment};
}

AnyBox::Address::SurfaceLayout AnyBox::Graphics::ComputeTextureSurface(const D3D12_RESOURCE_DESC& description) {
    if (description.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D) throw std::runtime_error("Texture allocation dimension is not implemented");
    if (description.Width == 0 || description.Width > 16384 || description.Height == 0 || description.Height > 16384 || description.DepthOrArraySize == 0 || description.DepthOrArraySize > 2048) throw std::invalid_argument("Texture allocation dimensions are invalid");
    if (description.MipLevels == 0) throw std::runtime_error("Implicit texture mip count is not implemented");
    if (description.MipLevels > std::bit_width(description.Width > description.Height ? description.Width : description.Height)) throw std::invalid_argument("Texture mip count exceeds its dimensions");
    if (description.SampleDesc.Count != 1 || description.SampleDesc.Quality != 0) throw std::runtime_error("Multisample texture allocation is not implemented");
    auto flags = static_cast<std::uint32_t>(description.Flags);
    if ((flags & ~0x30000DU) != 0 || ((flags & 1) != 0 && (flags & 0x300000) == 0)) throw std::runtime_error("Texture allocation metadata or resource flags are not implemented");
    if (description.Alignment != 0 && description.Alignment != 4096 && description.Alignment != 65536) throw std::runtime_error("Texture allocation alignment is not implemented");

    Address::SurfaceDescription input;
    input.width = static_cast<std::uint32_t>(description.Width);
    input.height = description.Height;
    input.slices = description.DepthOrArraySize;
    input.mipLevels = description.MipLevels;
    input.samples = 1;
    input.fragments = 1;
    input.bitsPerElement = BitsPerPixel(description.Format);
    input.swizzleMode = SwizzleMode(description, input.bitsPerElement);
    input.resourceType = 1;
    input.flags = (flags & 1) | ((flags & 8) == 0 ? 1U << 9 : 0) | ((flags & 4) != 0 ? 1U << 10 : 0) | (1U << 16);
    static const Address::Context context({13, 0x8F, 0x28, 0});
    return context.Compute(input);
}
