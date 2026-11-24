#include "Shader.hpp"
#include <anybox/shader/Requirements.hpp>

void AnyBox::Graphics::ValidateComputeShader(Device& device, std::span<const std::byte> code) {
    auto required = Shader::GetComputeWaveSize(code);
    if (!required) return;
    auto limits = device.GetWaveLimits();
    if (limits[0] != *required || limits[1] != *required) throw std::runtime_error("Xbox shader wave size requires explicit subgroup control on this adapter");
}
