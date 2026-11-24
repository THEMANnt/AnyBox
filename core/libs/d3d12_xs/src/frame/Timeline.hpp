#pragma once

#include <chrono>
#include <stdexcept>

namespace AnyBox::Graphics {
using FrameTime = std::chrono::steady_clock::time_point;

inline FrameTime NextFrameDeadline(FrameTime base, std::chrono::microseconds interval, std::chrono::microseconds offset, FrameTime after) {
    if (interval.count() <= 0 || offset.count() < 0 || offset >= interval) throw std::invalid_argument("Xbox frame deadline period or offset is invalid");
    auto first = base + offset;
    if (after < first) return first;
    return first + interval * ((after - first) / interval + 1);
}
}
