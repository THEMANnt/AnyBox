#include "Configuration.hpp"
#include "../dxgi/Object.hpp"
#include "../host/Display.hpp"
#include <anybox/Unimplemented.hpp>

namespace {
bool RequiresHighRefresh(std::uint32_t interval) {
    if (interval - 8333 <= 9 || interval - 25000 <= 25 || interval - 41667 <= 41) return true;
    if (interval - 16667 <= 16 || interval - 33333 <= 34 || interval - 50000 <= 49 || interval - 66667 <= 67) return false;
    throw std::invalid_argument("Xbox frame interval is outside the supported refresh periods");
}

bool SupportsHighRefresh(AnyBox::Graphics::Device* device) {
    using namespace AnyBox::Graphics;
    auto adapter = GetHostAdapter(device);
    IDXGIOutput* output = nullptr;
    AnyBox::Windows::CheckHResult(adapter->EnumOutputs(0, &output), "Xbox frame interval output lookup");
    ComOwner<IDXGIOutput> owner(output);
    constexpr GUID interfaceId{0x00CDDEA8, 0x939B, 0x4B83, {0xA3, 0x40, 0xA6, 0x85, 0x22, 0x66, 0x66, 0xCC}};
    void* extended = nullptr;
    AnyBox::Windows::CheckHResult(owner->QueryInterface(interfaceId, &extended), "Xbox frame interval display interface");
    ComOwner<IDXGIOutput1> display(static_cast<IDXGIOutput1*>(extended));
    auto modes = GetHostDisplayModes(display.get(), DXGI_FORMAT_UNKNOWN, 0);
    for (const auto& mode : modes) {
        if (mode.RefreshRate.Denominator == 0) throw std::runtime_error("Display refresh denominator is zero");
        if (mode.RefreshRate.Numerator == 120ull * mode.RefreshRate.Denominator) return true;
    }
    return false;
}
}

HRESULT AnyBox::Graphics::Device::SetFrameIntervalX(Device* self, void* output, std::uint32_t intervalMicroseconds, std::uint32_t periodicity, std::uint32_t flags) {
    if (output || flags != 0) ANYBOX_NOT_IMPLEMENTED();
    if (RequiresHighRefresh(intervalMicroseconds) && !SupportsHighRefresh(self)) throw std::invalid_argument("Xbox frame interval requires a display mode at 120 Hz");
    std::scoped_lock lock(self->frameMutex);
    self->frameConfiguration = {intervalMicroseconds, periodicity};
    return S_FALSE;
}
