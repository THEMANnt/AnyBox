#include "Slices.hpp"
#include <Translation/TranslationContext.hpp>
#include <algorithm>
#include <iterator>
#include <stdexcept>

void AnyBox::Shader::EmitNativeLegacyFma(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrBlock& block, const ShaderRecompiler::RdnaInstruction& source, const ShaderRecompiler::TranslateOptions& options, const ShaderRecompiler::RoundingState& rounding, const NativeWindowMode& window) {
    using namespace ShaderRecompiler;
    TranslationContext context(program, block, NumVectorRegs);
    context.SetFloatMode(options.floatMode);
    context.SetEntryRounding(rounding);
    context.SetFloat32DenormPreserve(options.float32DenormPreserve);
    context.SetFloat32FlushToZero(options.float32FlushToZero);
    context.SetFloat32FlushDeclared(options.float32FlushDeclared);
    context.SetSharedVectorRegisters(options.sharedVectorBase, options.sharedVectorCount);
    context.SetVectorWindowActive(window.active);
    if (window.synchronizeBefore) context.SyncVectorWindow(window.active);
    auto instruction = source;
    instruction.op = RdnaOpcode::VFmaF32;
    instruction.opcodeId = 0x14B;
    context.TranslateInstruction(instruction);
    auto& instructions = block.Instructions();
    auto fma = std::find_if(instructions.begin(), instructions.end(), [](auto* value) { return value->Opcode() == IrOpcode::FPFma32; });
    if (fma == instructions.end() || std::count_if(instructions.begin(), instructions.end(), [](auto* value) { return value->Opcode() == IrOpcode::FPFma32; }) != 1) throw std::logic_error("Native legacy FMA translation did not produce one fused operation");
    std::vector<IrValue*> consumers(fma, instructions.end());
    auto* lhs = (*fma)->Argument(0);
    auto* rhs = (*fma)->Argument(1);
    auto last = std::prev(instructions.end());
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    auto& lhsZero = builder.IEqual(builder.BitwiseAnd(builder.BitCastU32(*lhs), builder.Constant(0x7FFFFFFF)), builder.Constant(0));
    auto& rhsZero = builder.IEqual(builder.BitwiseAnd(builder.BitCastU32(*rhs), builder.Constant(0x7FFFFFFF)), builder.Constant(0));
    auto& zero = builder.LogicalOr(lhsZero, rhsZero);
    auto& protectedLhs = builder.Emit(IrOpcode::SelectF32, IrType::F32, {&zero, &builder.ConstantF32(0), lhs});
    auto& protectedRhs = builder.Emit(IrOpcode::SelectF32, IrType::F32, {&zero, &builder.ConstantF32(0), rhs});
    instructions.splice(fma, instructions, std::next(last), instructions.end());
    for (auto* value : consumers) {
        for (std::size_t argument = 0; argument < value->ArgumentCount(); ++argument) {
            auto* original = value->Argument(argument);
            if (original == lhs) value->ReplaceArgument(argument, &protectedLhs);
            else if (original == rhs) value->ReplaceArgument(argument, &protectedRhs);
        }
    }
}
