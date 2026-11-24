#pragma once

#include "../lowering/Buffer.hpp"

namespace AnyBox::Shader::Dxil {
BufferAddress ReadBufferArguments(const BufferBinding& binding, std::uint32_t first, ShaderRecompiler::IrValue& index, ShaderRecompiler::IrBuilder& builder);
}
