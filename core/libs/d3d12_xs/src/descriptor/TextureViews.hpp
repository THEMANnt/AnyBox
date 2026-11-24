#pragma once

#include "../resource/Resource.hpp"

namespace AnyBox::Graphics {
void CreateTextureShaderView(Resource& resource, const D3D12_SHADER_RESOURCE_VIEW_DESC* description, std::uint64_t destination);
void CreateTextureUnorderedView(Resource& resource, void* counterResource, const D3D12_UNORDERED_ACCESS_VIEW_DESC* description, std::uint64_t destination);
}
