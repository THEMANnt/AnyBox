#pragma once

#include "XmaContext.hpp"
#include <AcpHalTypes.hpp>
#include <array>
#include <memory>
#include <mutex>
#include <vector>

namespace AnyBox::AcpHal {
struct ShapeRuntime {
    std::mutex mutex;
    std::array<void*, 5> allocations{};
    AcpHalShapeContexts descriptor{};
    std::vector<std::unique_ptr<XmaContext>> decoders;
    std::uint64_t sequence = 0;
    bool allocated = false;
};

ShapeRuntime& GetShapeRuntime();
void SubmitShapeCommand(std::uint32_t type, const void* data);
}
