#pragma once

#include "Dxgi.hpp"
#include <vector>

namespace AnyBox::Graphics {
std::vector<DXGI_MODE_DESC1> GetHostDisplayModes(IDXGIOutput1* output, DXGI_FORMAT format, UINT flags);
}
