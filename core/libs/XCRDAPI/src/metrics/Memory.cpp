#include "Memory.hpp"
#include "Counters.hpp"

#include <cstdlib>
#include <exception>

namespace {

struct alignas(std::max_align_t) CodecAllocation {
    std::size_t size;
};

}

void AnyBox::Storage::AddMemory(std::size_t size) noexcept {
    auto& counters = GetCounters();
    auto current = counters.currentMemory.fetch_add(size, std::memory_order_relaxed) + size;
    UpdateMaximum(counters.maximumMemory, current);
}

void AnyBox::Storage::RemoveMemory(std::size_t size) noexcept {
    if (GetCounters().currentMemory.fetch_sub(size, std::memory_order_relaxed) < size) std::terminate();
}

void* AnyBox::Storage::AllocateCodecMemory(std::size_t size) noexcept {
    if (size > std::numeric_limits<std::size_t>::max() - sizeof(CodecAllocation)) return nullptr;
    auto total = size + sizeof(CodecAllocation);
    auto* memory = std::malloc(total);
    if (!memory) return nullptr;
    auto* header = std::construct_at(static_cast<CodecAllocation*>(memory), CodecAllocation{total});
    AddMemory(total);
    return header + 1;
}

void AnyBox::Storage::FreeCodecMemory(void* memory) noexcept {
    if (!memory) return;
    auto* header = static_cast<CodecAllocation*>(memory) - 1;
    RemoveMemory(header->size);
    std::free(header);
}
