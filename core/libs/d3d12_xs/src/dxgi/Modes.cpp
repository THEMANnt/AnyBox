#include "Output.hpp"
#include "../host/Display.hpp"
#include <algorithm>

HRESULT AnyBox::Graphics::DxgiOutput::GetDisplayModeList1(DXGI_FORMAT format, UINT flags, UINT* count, DXGI_MODE_DESC1* modes) {
    static_assert(sizeof(DXGI_MODE_DESC1) == 32);
    if (!count) throw std::invalid_argument("DXGI display mode count output is null");
    auto available = GetHostDisplayModes(host.get(), format, flags);
    auto capacity = modes ? *count : 0;
    if (available.size() > std::numeric_limits<UINT>::max()) throw std::overflow_error("DXGI display mode count overflows");
    *count = static_cast<UINT>(available.size());
    if (!modes) return S_OK;
    if (capacity < *count) throw std::length_error("DXGI display mode buffer is too small");
    std::ranges::copy(available, modes);
    return S_OK;
}
