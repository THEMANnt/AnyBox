#include <anybox/descriptor/ColorTarget.hpp>
#include <algorithm>
#include <stdexcept>

namespace {
struct ColorFormat { std::uint32_t id; std::uint32_t information; std::uint32_t control; };

ColorFormat GetFormat(std::uint32_t id) {
    constexpr std::array<ColorFormat, 24> formats{{
        {77, 0x28738, 0}, {75, 0x70438, 0}, {76, 0x70538, 0},
        {71, 0x28730, 0}, {65, 0x28030, 0}, {69, 0x70430, 0},
        {66, 0x28130, 0}, {70, 0x70530, 0}, {64, 0x2872C, 0x20000},
        {62, 0x7042C, 0x20000}, {63, 0x7052C, 0x20000}, {56, 0x28028, 0},
        {130, 0x28628, 0}, {60, 0x70428, 0}, {57, 0x28128, 0},
        {61, 0x70528, 0}, {29, 0x28714, 0x20000}, {23, 0x28014, 0x20000},
        {22, 0x28710, 0x20000}, {20, 0x70410, 0x20000}, {21, 0x70510, 0x20000},
        {13, 0x28708, 0x20000}, {7, 0x28008, 0x20000}, {1, 0x28004, 0x20000}
    }};
    for (const auto& format : formats) if (format.id == id) return format;
    throw std::runtime_error("Color target hardware format is not implemented");
}
}

AnyBox::Descriptor::ColorTargetWords AnyBox::Descriptor::EncodeColorTarget(const Texture2DDescription& texture, const ColorTargetViewDescription& view, const SurfacePitch& pitch) {
    TextureViewDescription selected;
    selected.address = view.address;
    selected.firstMip = view.mip;
    selected.mipCount = 1;
    selected.firstLayer = view.firstLayer;
    selected.layerCount = view.layerCount;
    auto words = EncodeTexture2D(texture, selected);
    if (pitch.width < texture.width || pitch.height < texture.height || pitch.width > 16384 || pitch.height > 16384 || pitch.width % 8 != 0 || pitch.height % 8 != 0) throw std::invalid_argument("Color target surface pitch is invalid");
    auto format = GetFormat(texture.format);
    auto lastLayer = view.layerCount == UINT32_MAX ? texture.layers - 1 : view.firstLayer + view.layerCount - 1;
    auto row = pitch.width / 8 - 1;
    auto slice = pitch.width * pitch.height / 64 - 1;
    return {
        std::max(1U, texture.width >> view.mip) | (std::max(1U, texture.height >> view.mip) << 16),
        words[0],
        words[1] & 255,
        (texture.height - 1) | ((texture.width - 1) << 14) | ((texture.mipLevels - 1) << 28),
        view.firstLayer | (lastLayer << 13) | (view.mip << 26),
        format.information,
        format.control,
        0,
        row | (row << 20),
        slice,
        0x0DC00000 | (texture.swizzleMode << 14) | (texture.layers - 1),
        view.mip
    };
}
