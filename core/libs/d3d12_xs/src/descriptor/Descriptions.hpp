#pragma once

#include <d3d12.h>
#include <array>
#include <cstddef>
#include <cstdint>

namespace AnyBox::Graphics {
struct XboxShaderViewDescription {
    D3D12_SHADER_RESOURCE_VIEW_DESC view;
    std::uint64_t address;
    std::array<std::uint32_t, 4> parameters;
};

static_assert(sizeof(D3D12_RESOURCE_DESC) == 56);
static_assert(offsetof(D3D12_RESOURCE_DESC, Flags) == 48);
static_assert(sizeof(XboxShaderViewDescription) == 64);
static_assert(offsetof(XboxShaderViewDescription, address) == 40);
static_assert(offsetof(XboxShaderViewDescription, parameters) == 48);
}
