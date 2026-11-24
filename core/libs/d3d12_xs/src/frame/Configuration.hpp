#pragma once

#include <cstdint>

namespace AnyBox::Graphics {
struct FrameConfiguration {
    std::uint32_t intervalMicroseconds = 0;
    std::uint32_t periodicity = 0;
    bool operator==(const FrameConfiguration&) const = default;
};
}
