#include <anybox/descriptor/Texture.hpp>
#include <stdexcept>

namespace {
std::uint32_t ComponentMapping(std::uint32_t format) {
    switch (format) {
        case 77: case 75: case 76: case 71: case 65: case 69: case 66:
        case 70: case 56: case 130: case 60: case 57: case 61: return 0x1688;
        case 64: case 62: case 63: case 29: case 23: return 0x1B08;
        case 22: case 20: case 21: case 13: case 7: case 1: return 0x1B20;
        default: throw std::runtime_error("Storage texture hardware format is not implemented");
    }
}
}

AnyBox::Descriptor::ImageWords AnyBox::Descriptor::EncodeStorageTexture2D(const Texture2DDescription& texture, const StorageViewDescription& view) {
    TextureViewDescription selected;
    selected.address = view.address;
    selected.firstMip = view.mip;
    selected.mipCount = 1;
    selected.firstLayer = view.firstLayer;
    selected.layerCount = view.layerCount;
    selected.componentMapping = ComponentMapping(texture.format);
    auto words = EncodeTexture2D(texture, selected);
    words[5] &= ~0x400000U;
    return words;
}
