#include "Tests.hpp"

#include <stdexcept>

AnyBox::Tests::PhysicalBuffer::PhysicalBuffer(std::size_t count) : frames(count) {
    if (count == 0) throw std::invalid_argument("Physical test buffer is empty");
    XMemAllocatePhysicalPages(0, &count, frames.data());
    allocated = true;
    try {
        memory = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, count * AnyBox::PhysicalPageSize, 0x20402000, 0x23, 0x2004));
        Map(frames);
    } catch (...) {
        if (memory) releaseWindow();
        XMemFreePhysicalPages(frames.size(), frames.data());
        throw;
    }
}

AnyBox::Tests::PhysicalBuffer::~PhysicalBuffer() {
    Unmap();
    if (allocated) FreePages();
    releaseWindow();
}

std::span<std::byte> AnyBox::Tests::PhysicalBuffer::GetMemory() const {
    if (!mapped) throw std::logic_error("Physical test buffer is not mapped");
    return {memory, frames.size() * AnyBox::PhysicalPageSize};
}

std::span<const std::uint64_t> AnyBox::Tests::PhysicalBuffer::GetFrames() const { return frames; }

void AnyBox::Tests::PhysicalBuffer::Map(std::span<const std::uint64_t> order) {
    if (!allocated || order.size() != frames.size()) throw std::invalid_argument("Invalid physical test mapping");
    XMemMapPhysicalPages(memory, order.size(), order.data());
    mapped = true;
}

void AnyBox::Tests::PhysicalBuffer::Unmap() {
    if (!mapped) return;
    XMemMapPhysicalPages(memory, frames.size(), nullptr);
    mapped = false;
}

void AnyBox::Tests::PhysicalBuffer::FreePages() {
    if (!XMemFreePhysicalPages(frames.size(), frames.data())) throw std::runtime_error("Physical test page release failed");
    allocated = false;
}

void AnyBox::Tests::PhysicalBuffer::releaseWindow() {
    auto* current = memory;
    auto* end = memory + frames.size() * AnyBox::PhysicalPageSize;
    while (current < end) {
        MEMORY_BASIC_INFORMATION information{};
        if (!VirtualQuery(current, &information, sizeof(information))) Windows::ThrowLastError("VirtualQuery physical test window");
        if (information.State != MEM_RESERVE || information.BaseAddress != current || information.RegionSize == 0) throw std::runtime_error("Invalid physical test placeholder");
        auto size = information.RegionSize;
        if (!VirtualFree(current, 0, MEM_RELEASE)) Windows::ThrowLastError("VirtualFree physical test window");
        current += size;
    }
    memory = nullptr;
}
