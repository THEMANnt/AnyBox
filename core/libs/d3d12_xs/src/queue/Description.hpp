#pragma once

#include <cstdint>

namespace AnyBox::Graphics {
struct XboxQueueDescription {
    std::uint32_t type;
    std::uint32_t flags;
    std::uint32_t engineIndex;
    std::uint32_t queueIndex;
};

static_assert(sizeof(XboxQueueDescription) == 16);
}
