#pragma once

#include "Texture.hpp"

namespace AnyBox::Descriptor {
using ColorTargetWords = std::array<std::uint32_t, 12>;

struct ColorTargetViewDescription {
    std::uint64_t address;
    std::uint32_t mip;
    std::uint32_t firstLayer;
    std::uint32_t layerCount;
};

struct SurfacePitch {
    std::uint32_t width;
    std::uint32_t height;
};

ColorTargetWords EncodeColorTarget(const Texture2DDescription& texture, const ColorTargetViewDescription& view, const SurfacePitch& pitch);
}
