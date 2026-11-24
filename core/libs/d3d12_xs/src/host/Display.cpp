#include "Display.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <array>
#include <numeric>
#include <stdexcept>

namespace {
std::vector<DXGI_MODE_DESC1> Enumerate(IDXGIOutput1* output, DXGI_FORMAT format, UINT flags) {
    UINT count = 0;
    auto result = output->GetDisplayModeList1(format, flags, &count, nullptr);
    if (result == DXGI_ERROR_NOT_FOUND) return {};
    AnyBox::Windows::CheckHResult(result, "DXGI display mode count");
    std::vector<DXGI_MODE_DESC1> modes(count);
    if (count != 0) {
        AnyBox::Windows::CheckHResult(output->GetDisplayModeList1(format, flags, &count, modes.data()), "DXGI display mode enumeration");
        modes.resize(count);
    }
    for (auto& mode : modes) {
        if (mode.RefreshRate.Denominator == 0) throw std::runtime_error("DXGI display mode refresh denominator is zero");
        auto divisor = std::gcd(mode.RefreshRate.Numerator, mode.RefreshRate.Denominator);
        mode.RefreshRate.Numerator /= divisor;
        mode.RefreshRate.Denominator /= divisor;
    }
    return modes;
}
}

std::vector<DXGI_MODE_DESC1> AnyBox::Graphics::GetHostDisplayModes(IDXGIOutput1* output, DXGI_FORMAT format, UINT flags) {
    if (!output) throw std::invalid_argument("DXGI display mode source is null");
    constexpr UINT supported = DXGI_ENUM_MODES_INTERLACED | DXGI_ENUM_MODES_SCALING | DXGI_ENUM_MODES_STEREO | DXGI_ENUM_MODES_DISABLED_STEREO;
    if ((flags & ~supported) != 0) throw std::invalid_argument("Unsupported DXGI display enumeration flags");
    if (format != DXGI_FORMAT_UNKNOWN) return Enumerate(output, format, flags);
    constexpr std::array formats{DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_R10G10B10A2_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT};
    std::vector<DXGI_MODE_DESC1> available;
    for (auto candidate : formats) {
        auto values = Enumerate(output, candidate, flags);
        available.insert(available.end(), values.begin(), values.end());
    }
    return available;
}
