#include "Tests.hpp"
#include <xmem.hpp>

AnyBox::Vulkan::Tests::Storage::Storage() {
    try {
        auto count = frames.size();
        Require(XMemAllocatePhysicalPages(0x280, &count, frames.data()) == 1 && count == frames.size(), "Vulkan physical test page allocation failed");
        allocated = true;
        count = 1;
        Require(XMemAllocatePhysicalPages(0x280, &count, &extra) == 1 && count == 1, "Vulkan physical test extra page allocation failed");
        extraAllocated = true;
        address = static_cast<std::byte*>(XMemVirtualAlloc(nullptr, frames.size() * PhysicalPageSize, 0x20402000, 0x23, 0x2004));
        if (!address) throw std::runtime_error("Vulkan physical test window is null");
    } catch (...) { release(); throw; }
}

AnyBox::Vulkan::Tests::Storage::~Storage() { release(); }

void AnyBox::Vulkan::Tests::Storage::release() {
    if (address) {
        XMemMapPhysicalPages(address, frames.size(), nullptr);
        for (std::size_t index = 0; index < frames.size(); ++index) {
            if (!VirtualFree(address + index * PhysicalPageSize, 0, MEM_RELEASE)) throw std::runtime_error("Vulkan physical test placeholder release failed");
        }
        address = nullptr;
    }
    if (extraAllocated) {
        XMemFreePhysicalPages(1, &extra);
        extraAllocated = false;
    }
    if (allocated) {
        XMemFreePhysicalPages(frames.size(), frames.data());
        allocated = false;
    }
}

void AnyBox::Vulkan::Tests::Storage::Map(std::span<const std::uint64_t> values) {
    if (values.size() != frames.size()) throw std::invalid_argument("Vulkan physical test mapping has an invalid page count");
    Require(XMemMapPhysicalPages(address, values.size(), values.data()) == address, "Vulkan physical test mapping failed");
}

std::span<std::byte> AnyBox::Vulkan::Tests::Storage::GetMemory() const noexcept { return {address, frames.size() * PhysicalPageSize}; }
