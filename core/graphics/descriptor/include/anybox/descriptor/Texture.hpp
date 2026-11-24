#pragma once

#include <array>
#include <cstdint>

namespace AnyBox::Descriptor {
using ImageWords = std::array<std::uint32_t, 8>;

struct Texture2DDescription {
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t layers;
    std::uint32_t mipLevels;
    std::uint32_t format;
    std::uint32_t swizzleMode;
};

struct TextureViewDescription {
    std::uint64_t address = 0;
    std::uint32_t firstMip = 0;
    std::uint32_t mipCount = UINT32_MAX;
    std::uint32_t firstLayer = 0;
    std::uint32_t layerCount = UINT32_MAX;
    std::uint32_t componentMapping = 0x1688;
    float minimumLod = 0;
};

ImageWords EncodeTexture2D(const Texture2DDescription& texture, const TextureViewDescription& view);

struct StorageViewDescription {
    std::uint64_t address;
    std::uint32_t mip;
    std::uint32_t firstLayer;
    std::uint32_t layerCount;
};

ImageWords EncodeStorageTexture2D(const Texture2DDescription& texture, const StorageViewDescription& view);
}
