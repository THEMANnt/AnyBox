#pragma once

#include "../../src/resource/Resource.hpp"

namespace AnyBox::Graphics::Tests {
void CheckTextureShaderViews(Device* device, Resource* resource);
void CheckTextureUnorderedViews(Device* device, Resource* resource);
}
