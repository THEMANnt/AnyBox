#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>

namespace AnyBox::Shader {
bool MaskDominates(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrBlock* first, const ShaderRecompiler::IrBlock* second);
bool MaskNarrows(const ShaderRecompiler::IrValue* value, const ShaderRecompiler::IrValue* phi, ShaderRecompiler::IrOpcode operation, unsigned depth, unsigned& steps);
}
