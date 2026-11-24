#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace AnyBox::Shader {
enum class ResourceClass : std::uint8_t { ShaderResource, UnorderedAccess, ConstantBuffer, Sampler };
enum class BufferKind : std::uint8_t { Raw, Structured, Constant };

struct BufferBinding {
    ResourceClass resourceClass;
    std::uint32_t rangeId;
    std::uint32_t space;
    std::uint32_t firstRegister;
    std::uint32_t count;
    BufferKind kind;
    std::uint32_t stride = 0;
    std::uint32_t byteSize = 0;
    bool globallyCoherent = false;
    bool hasCounter = false;
    bool rasterizerOrdered = false;
    bool operator==(const BufferBinding&) const = default;
};

std::vector<BufferBinding> ReadDxilBufferBindings(std::span<const std::byte> code);
}
