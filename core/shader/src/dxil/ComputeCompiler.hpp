#pragma once

#include <anybox/shader/ComputeCompiler.hpp>

namespace AnyBox::Shader::Dxil {
using AnyBox::Shader::ComputeCompilation;
ComputeCompilation CompileCompute(ShaderRecompiler::IrProgram& program, const ShaderRecompiler::ShaderComputeInputInfo& input, const ShaderRecompiler::SpirvTargetOptions& target, const ShaderRecompiler::BindingLayout& layout);
}
