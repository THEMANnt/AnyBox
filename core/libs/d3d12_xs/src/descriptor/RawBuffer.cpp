#include "RawBuffer.hpp"
#include <stdexcept>

std::array<std::uint32_t, 8> AnyBox::Graphics::EncodeRawBuffer(std::uint64_t baseAddress, std::uint64_t width, std::uint64_t firstElement, std::uint32_t elementCount, std::uint32_t componentMapping) {
    if (width > UINT32_MAX || width % 4 != 0 || firstElement > width / 4) throw std::out_of_range("Xbox raw buffer view extent or first element is invalid");
    auto offset = firstElement * 4;
    auto count = elementCount == UINT32_MAX ? width - offset : static_cast<std::uint64_t>(elementCount) * 4;
    if (count > UINT32_MAX || count > width - offset) throw std::out_of_range("Xbox raw buffer view exceeds its resource extent");
    if (baseAddress > 0xFFFFFFFFFFFFULL || offset > 0xFFFFFFFFFFFFULL - baseAddress || baseAddress % 4 != 0) throw std::invalid_argument("Xbox raw buffer view address cannot be encoded");
    auto address = baseAddress + offset;
    constexpr std::array<std::uint32_t, 6> selectors{4, 5, 6, 7, 0, 1};
    std::uint32_t swizzle = 0;
    for (unsigned component = 0; component < 4; ++component) {
        auto selector = (componentMapping >> (component * 3)) & 7;
        if (selector >= selectors.size()) throw std::invalid_argument("Xbox buffer view component selector is invalid");
        swizzle |= selectors[selector] << (component * 3);
    }
    return {static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32), static_cast<std::uint32_t>(count), 0x30014000 | swizzle, 0, 0, 0, 0};
}
