#include <anybox/descriptor/ColorTarget.hpp>
#include <stdexcept>

void CheckColorTargets() {
    using namespace AnyBox::Descriptor;
    ColorTargetViewDescription view{0x121234567800, 0, 0, 1};
    auto words = EncodeColorTarget({3840, 2160, 1, 1, 56, 27}, view, {3840, 2176});
    constexpr ColorTargetWords reference{0x08700F00, 0x12345678, 0x12, 0x03BFC86F, 0, 0x28028, 0, 0, 0x1DF001DF, 0x1FDFF, 0x0DC6C000, 0};
    if (words != reference) throw std::runtime_error("Color target registers differ from their native reference");
    view.mip = 8;
    view.firstLayer = 5;
    words = EncodeColorTarget({256, 256, 6, 9, 56, 27}, view, {256, 256});
    constexpr ColorTargetWords array{0x10001, 0x12345678, 0x12, 0x803FC0FF, 0x2000A005, 0x28028, 0, 0, 0x01F0001F, 0x3FF, 0x0DC6C005, 8};
    if (words != array) throw std::runtime_error("Color target array registers differ from their native reference");
    view.firstLayer = 6;
    try { EncodeColorTarget({256, 256, 6, 9, 56, 27}, view, {256, 256}); }
    catch (const std::out_of_range&) { return; }
    throw std::runtime_error("Color target accepted an out-of-range array slice");
}
