#pragma once

#include <RdnaDecoder/RdnaProgram.hpp>
#include <optional>

namespace AnyBox::Shader {
std::optional<ShaderRecompiler::RdnaProgram> RemoveUnusedScratchWrites(const ShaderRecompiler::RdnaProgram& program);
}
