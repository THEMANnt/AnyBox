#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>
#include <cstddef>

namespace AnyBox::Shader {
bool NativeMaskImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition);
std::size_t SimplifyFirstLaneGuards(ShaderRecompiler::IrProgram& program);
}
