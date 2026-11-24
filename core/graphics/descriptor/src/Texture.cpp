#include <anybox/descriptor/Texture.hpp>
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>

AnyBox::Descriptor::ImageWords AnyBox::Descriptor::EncodeTexture2D(const Texture2DDescription& texture, const TextureViewDescription& view) {
    if (texture.width == 0 || texture.width > 16384 || texture.height == 0 || texture.height > 16384 || texture.layers == 0 || texture.layers > 2048) throw std::invalid_argument("Texture descriptor dimensions are invalid");
    auto maximumMips = static_cast<std::uint32_t>(std::bit_width(std::max(texture.width, texture.height)));
    if (texture.mipLevels == 0 || texture.mipLevels > maximumMips) throw std::invalid_argument("Texture descriptor mip count is invalid");
    if (texture.format == 0 || texture.format > 0x11F) throw std::invalid_argument("Texture descriptor hardware format is invalid");
    if (texture.swizzleMode != 5 && texture.swizzleMode != 21 && texture.swizzleMode != 27) throw std::runtime_error("Texture descriptor swizzle mode is not implemented");
    if (view.address > 0xFFFFFFFFFFFFULL || view.address % 256 != 0) throw std::invalid_argument("Texture descriptor address is not representable");
    if (view.firstMip >= texture.mipLevels || view.firstLayer >= texture.layers) throw std::out_of_range("Texture view starts outside its resource");
    auto mipCount = view.mipCount == UINT32_MAX ? texture.mipLevels - view.firstMip : view.mipCount;
    auto layerCount = view.layerCount == UINT32_MAX ? texture.layers - view.firstLayer : view.layerCount;
    if (mipCount == 0 || mipCount > texture.mipLevels - view.firstMip || layerCount == 0 || layerCount > texture.layers - view.firstLayer) throw std::out_of_range("Texture view exceeds its resource");
    if (!std::isfinite(view.minimumLod) || view.minimumLod < 0) throw std::invalid_argument("Texture view minimum LOD is invalid");

    constexpr std::array<std::uint32_t, 6> selectors{4, 5, 6, 7, 0, 1};
    std::uint32_t mapping = 0;
    for (unsigned component = 0; component < 4; ++component) {
        auto selector = (view.componentMapping >> (component * 3)) & 7;
        if (selector >= selectors.size()) throw std::invalid_argument("Texture view component selector is invalid");
        mapping |= selectors[selector] << (component * 3);
    }
    auto width = texture.width - 1;
    auto minimumLod = static_cast<std::uint32_t>(std::min(4095.0, static_cast<double>(view.minimumLod) * 256));
    minimumLod = std::max(minimumLod, view.firstMip * 256);
    auto lastMip = view.firstMip + mipCount - 1;
    auto lastLayer = view.firstLayer + layerCount - 1;
    return {
        static_cast<std::uint32_t>(view.address >> 8),
        ((width & 3) << 30) | (texture.format << 20) | (minimumLod << 8) | static_cast<std::uint32_t>(view.address >> 40),
        0x80000000 | ((texture.height - 1) << 14) | (width >> 2),
        0xD0000000 | (texture.swizzleMode << 20) | (lastMip << 16) | (view.firstMip << 12) | mapping,
        (view.firstLayer << 16) | lastLayer,
        0x80400000 | ((texture.mipLevels - 1) << 4),
        0,
        0
    };
}
