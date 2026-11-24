#include <AcpHal.hpp>
#include "../memory/Heap.hpp"
#include "Runtime.hpp"

#include <array>
#include <cstring>
#include <mutex>
#include <stdexcept>

namespace {

void Release(std::array<void*, 5>& allocations) {
    for (auto*& allocation : allocations) {
        ApuFree(allocation);
        allocation = nullptr;
    }
}

}

AnyBox::AcpHal::ShapeRuntime& AnyBox::AcpHal::GetShapeRuntime() {
    static ShapeRuntime runtime;
    return runtime;
}

std::int32_t AcpHalAllocateShapeContexts(AcpHalShapeContexts* shapeContexts) noexcept(false) {
    if (!shapeContexts) throw std::invalid_argument("SHAPE context descriptor is null");
    const auto& counts = shapeContexts->counts;
    for (std::size_t i = 0; i < 4; ++i) {
        if (counts[i] > 4096) throw std::invalid_argument("SHAPE context count exceeds its limit");
    }
    if (counts[4] > 512 || counts[5] > 3584) throw std::invalid_argument("SHAPE context count exceeds its limit");
    if ((counts[0] || counts[5]) && counts[0] != counts[4] + counts[5]) throw std::invalid_argument("Inconsistent SHAPE context counts");
    auto& runtime = AnyBox::AcpHal::GetShapeRuntime();
    std::lock_guard lock(runtime.mutex);
    if (runtime.allocated) throw std::logic_error("SHAPE contexts already exist");
    std::vector<std::unique_ptr<AnyBox::AcpHal::XmaContext>> decoders(counts[4]);
    AnyBox::AcpHal::GetHeap().Initialize();
    std::array<void*, 5> replacement{};
    AcpHalShapeContexts result = *shapeContexts;
    result.contexts.fill(nullptr);
    result.apuAddresses.fill(0);
    constexpr std::array<std::uint32_t, 4> strides{32, 160, 32, 16};
    try {
        for (std::size_t i = 0; i < 4; ++i) {
            if (counts[i] == 0) continue;
            ApuAlloc(&replacement[i], &result.apuAddresses[i], counts[i] * strides[i], strides[i] == 32 ? 32 : 16, 2);
            result.contexts[i] = replacement[i];
            std::memset(replacement[i], 0, counts[i] * strides[i]);
        }
        auto sharedSize = counts[4] * 64 + counts[5] * 32;
        if (sharedSize) {
            ApuAlloc(&replacement[4], nullptr, sharedSize, 64, 2);
            std::memset(replacement[4], 0, sharedSize);
            if (counts[4]) result.contexts[4] = replacement[4];
            if (counts[5]) result.contexts[5] = static_cast<std::byte*>(replacement[4]) + counts[4] * 64;
            for (std::size_t i = 4; i < 6; ++i) result.apuAddresses[i] = ApuMapVirtualAddress(result.contexts[i]);
        }
    } catch (...) {
        Release(replacement);
        throw;
    }
    runtime.allocations = replacement;
    runtime.descriptor = result;
    runtime.decoders = std::move(decoders);
    runtime.allocated = true;
    *shapeContexts = result;
    return 0;
}

std::int32_t AcpHalReleaseShapeContexts() noexcept(false) {
    auto& runtime = AnyBox::AcpHal::GetShapeRuntime();
    std::lock_guard lock(runtime.mutex);
    if (!runtime.allocated) throw std::logic_error("SHAPE contexts do not exist");
    runtime.decoders.clear();
    Release(runtime.allocations);
    runtime.descriptor = {};
    runtime.allocated = false;
    return 0;
}
