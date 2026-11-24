#pragma once

#include <anybox/shader/ComputeCompiler.hpp>

namespace AnyBox::Shader {
bool OptimizeWaveReductions(std::vector<std::uint32_t>& code, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target);
}
