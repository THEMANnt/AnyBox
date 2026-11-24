#pragma once

#include <cstddef>
#include <cstdint>

namespace AnyBox::XMem {
void* AllocateHeapStorage(std::size_t size, std::uint64_t attributes, std::size_t alignment);
void FreeHeapStorage(void* allocation, std::uint64_t attributes, std::size_t alignment);
}
