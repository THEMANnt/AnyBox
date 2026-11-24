#pragma once

#include "Native.hpp"
#include <anybox/graphics/container/NativeCompute.hpp>
#include <IntermediateRepresentation/IrProgram.hpp>
#include <SpirvBackend/SpirvEmitter.hpp>
#include <Optimization/ShaderStageInputInfo.hpp>

namespace ShaderRecompiler { struct ShaderFloatMode; }

namespace AnyBox::Shader {
struct NativeComputeTranslation {
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::ShaderComputeInputInfo input;
};

NativeComputeTranslation LowerNativeCompute(const NativeFrontend& frontend, const Graphics::Container::NativeComputeState& state, const ShaderRecompiler::ShaderFloatMode& floatMode, const ShaderRecompiler::SpirvTargetOptions& target);
}
