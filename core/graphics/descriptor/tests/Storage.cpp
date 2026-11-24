#include <anybox/descriptor/Texture.hpp>
#include <stdexcept>

void CheckStorageViews() {
    using namespace AnyBox::Descriptor;
    StorageViewDescription view{0x121234567800, 0, 0, 1};
    auto words = EncodeStorageTexture2D({63, 17, 1, 1, 56, 27}, view);
    constexpr ImageWords single{0x12345678, 0x83800012, 0x8004000F, 0xD1B00FAC, 0, 0x80000000, 0, 0};
    if (words != single) throw std::runtime_error("Storage texture descriptor differs from its native reference");
    view.mip = 8;
    view.firstLayer = 5;
    words = EncodeStorageTexture2D({256, 256, 6, 9, 64, 27}, view);
    constexpr ImageWords array{0x12345678, 0xC4080012, 0x803FC03F, 0xD1B8822C, 0x50005, 0x80000080, 0, 0};
    if (words != array) throw std::runtime_error("Storage texture channel mapping or mip range differs from its native reference");
    view.layerCount = 2;
    try { EncodeStorageTexture2D({256, 256, 6, 9, 64, 27}, view); }
    catch (const std::out_of_range&) { return; }
    throw std::runtime_error("Storage texture accepted an invalid layer range");
}
