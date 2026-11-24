#pragma once

#include "../device/Device.hpp"

namespace AnyBox::Graphics {
class TextureMemory {
public:
    TextureMemory(std::uint64_t size, std::uint64_t alignment, D3D12_HEAP_FLAGS flags);
    ~TextureMemory();
    TextureMemory(const TextureMemory&) = delete;
    TextureMemory& operator=(const TextureMemory&) = delete;
    TextureMemory(TextureMemory&& other) noexcept;
    TextureMemory& operator=(TextureMemory&& other) = delete;
    void* GetAddress() const noexcept { return address; }
    std::uint64_t GetSize() const noexcept { return size; }

private:
    void* address = nullptr;
    std::uint64_t size;
    std::uint64_t attributes;
};
}
