#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>
#include <optional>

namespace AnyBox::Shader {
struct NativeBitPredicate {
    const ShaderRecompiler::IrValue* word;
    const ShaderRecompiler::IrValue* selector;
};

std::optional<NativeBitPredicate> ReadNativeBitPredicate(const ShaderRecompiler::IrValue* value);
std::optional<std::uint32_t> ReadNativeLaneMask(const ShaderRecompiler::IrValue* value);
bool NativeLowHalf(const ShaderRecompiler::IrValue* value);
bool NativeBitPredicateImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition, unsigned& steps);
bool NativeBallotPredicateImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition, unsigned& steps);
}
