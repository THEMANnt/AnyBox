#pragma once

#include "Handle.hpp"
#include <array>
#include <cstddef>
#include <cstdint>

namespace AnyBox::Graphics {
struct FrameAncillary {
    std::uint32_t count;
    const HANDLE* handles;
    std::int32_t* signaled;
};
static_assert(offsetof(FrameAncillary, handles) == 8);
static_assert(offsetof(FrameAncillary, signaled) == 16);
static_assert(sizeof(FrameAncillary) == 24);

struct FrameSignal {
    FrameHandle handle;
    bool semaphore = false;
    void Signal() const;
};

class FrameSignals {
public:
    explicit FrameSignals(const FrameAncillary* ancillary);
    void Signal() const;

private:
    std::array<FrameSignal, 7> signals;
    std::uint32_t count = 0;
};
}
