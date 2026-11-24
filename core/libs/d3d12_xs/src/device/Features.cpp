#include "Device.hpp"

std::array<std::uint32_t, 2> AnyBox::Graphics::Device::GetWaveLimits() {
    std::call_once(waveInitialization, [this] {
        D3D12_FEATURE_DATA_D3D12_OPTIONS1 features{};
        CheckGraphicsResult(host->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &features, sizeof(features)), "Query D3D12 wave capabilities");
        if (features.WaveOps) waveLimits = {features.WaveLaneCountMin, features.WaveLaneCountMax};
    });
    return waveLimits;
}
