#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>

namespace AnyBox::Shader {
void OptimizeNativeProgram(ShaderRecompiler::IrProgram& program, std::uint32_t waveSize);
}
