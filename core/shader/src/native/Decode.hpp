#pragma once

#include <RdnaDecoder/RdnaProgram.hpp>
#include <span>

namespace AnyBox::Shader {
ShaderRecompiler::RdnaProgram DecodeNativeProgram(std::span<const std::uint32_t> code);
}
