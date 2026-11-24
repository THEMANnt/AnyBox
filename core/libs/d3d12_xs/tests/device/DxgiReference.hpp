#pragma once

#include "../../src/dxgi/Object.hpp"

namespace AnyBox::Graphics::Tests {
ComOwner<IDXGIOutput1> GetReferenceOutput(LUID adapter);
}
