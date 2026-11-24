#pragma once

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <cstdint>
#include <anybox/address/Context.hpp>

namespace AnyBox::Graphics {
D3D12_RESOURCE_ALLOCATION_INFO ComputeTextureAllocation(const D3D12_RESOURCE_DESC& description);
std::uint32_t ComputeTextureSwizzleMode(const D3D12_RESOURCE_DESC& description);
Address::SurfaceLayout ComputeTextureSurface(const D3D12_RESOURCE_DESC& description);
}
