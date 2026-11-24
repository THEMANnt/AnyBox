#include <d3d12_xs.hpp>
#include "Configuration.hpp"
#include <xmem.hpp>
#include <anybox/Unimplemented.hpp>
#include <cstring>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <stdexcept>

namespace {
struct AlignedDelete {
    void operator()(std::byte* value) const noexcept { ::operator delete(value, std::align_val_t{0x10000}); }
};
using Memory = std::unique_ptr<std::byte, AlignedDelete>;

Memory Allocate(std::uint32_t size) {
    if (size == 0) return {};
    auto* allocation = static_cast<std::byte*>(::operator new(size, std::align_val_t{0x10000}));
    std::memset(allocation, 0, size);
    return Memory(allocation);
}

struct GraphicsMemory {
    std::mutex mutex;
    std::optional<D3D12VirtualMemoryConfiguration> configuration;
    Memory pageTables;
    Memory framePreserve;
};
}

namespace {
std::int32_t ConfigureMemory(D3D12VirtualMemoryConfiguration* configuration, bool retainExisting) {
    if (!configuration) throw std::invalid_argument("D3D12 virtual memory configuration is null");
    if ((configuration->flags & ~1u) != 0 || configuration->pageTablePageCount > 33) throw std::invalid_argument("Invalid D3D12 virtual memory configuration");
    if (configuration->flags != 0) ANYBOX_NOT_IMPLEMENTED();
    static GraphicsMemory memory;
    std::lock_guard lock(memory.mutex);
    if (memory.configuration) {
        const auto& previous = *memory.configuration;
        auto previousPages = previous.pageTablePageCount != 0 ? previous.pageTablePageCount : 5;
        if (!retainExisting && ((configuration->pageTablePageCount != 0 && configuration->pageTablePageCount != previousPages) || configuration->framePreserveBufferSize != previous.framePreserveBufferSize)) throw std::logic_error("D3D12 virtual memory has already been configured differently");
        configuration->backendData = previous.backendData;
        return 0x10000000;
    }

    auto pageTableBytes = configuration->pageTablePageCount != 0 ? configuration->pageTablePageCount << 22 : 0x1400000;
    auto pageTables = Allocate(pageTableBytes);
    auto framePreserve = Allocate(configuration->framePreserveBufferSize);
    auto result = *configuration;
    result.backendData = {reinterpret_cast<std::uintptr_t>(pageTables.get()), reinterpret_cast<std::uintptr_t>(framePreserve.get())};
    XMemGraphicsMemoryConfiguration registration{pageTableBytes, configuration->framePreserveBufferSize, result.backendData};
    XMemRegisterGraphicsMemoryConfiguration(&registration);
    memory.pageTables = std::move(pageTables);
    memory.framePreserve = std::move(framePreserve);
    memory.configuration = result;
    *configuration = result;
    return 0x10000000;
}
}

std::int32_t D3D12ConfigureVirtualMemoryX(D3D12VirtualMemoryConfiguration* configuration) noexcept(false) { return ConfigureMemory(configuration, false); }

void AnyBox::Graphics::EnsureGraphicsMemoryConfigured() {
    D3D12VirtualMemoryConfiguration configuration{};
    ConfigureMemory(&configuration, true);
}
