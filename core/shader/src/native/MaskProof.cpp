#include "FirstLane.hpp"
#include "BitProof.hpp"
#include "MaskFlow.hpp"

namespace {
using namespace ShaderRecompiler;
using namespace AnyBox::Shader;

bool Implies(const IrProgram& program, const IrValue* mask, const IrValue* condition, unsigned depth, unsigned& steps) {
    if (steps == 0) return false;
    --steps;
    mask = mask->Resolve();
    condition = condition->Resolve();
    if (mask == condition || (mask->HasImmediate() && !mask->ImmediateBool()) || (condition->HasImmediate() && condition->ImmediateBool())) return true;
    if (depth == 0) return false;
    if (NativeBitPredicateImplies(program, mask, condition, steps)) return true;
    if (NativeBallotPredicateImplies(program, mask, condition, steps)) return true;
    if (condition->Opcode() == IrOpcode::LogicalAnd && condition->ArgumentCount() == 2 && Implies(program, mask, condition->Argument(0), depth - 1, steps) && Implies(program, mask, condition->Argument(1), depth - 1, steps)) return true;
    if (condition->Opcode() == IrOpcode::LogicalOr && condition->ArgumentCount() == 2 && (Implies(program, mask, condition->Argument(0), depth - 1, steps) || Implies(program, mask, condition->Argument(1), depth - 1, steps))) return true;
    if (mask->Opcode() == IrOpcode::SelectU1 && mask->ArgumentCount() == 3) return Implies(program, mask->Argument(1), condition, depth - 1, steps) && Implies(program, mask->Argument(2), condition, depth - 1, steps);
    if (mask->Opcode() == IrOpcode::LogicalAnd && mask->ArgumentCount() == 2) return Implies(program, mask->Argument(0), condition, depth - 1, steps) || Implies(program, mask->Argument(1), condition, depth - 1, steps);
    if (mask->Opcode() == IrOpcode::LogicalOr && mask->ArgumentCount() == 2) return Implies(program, mask->Argument(0), condition, depth - 1, steps) && Implies(program, mask->Argument(1), condition, depth - 1, steps);
    if (!mask->IsPhi() || mask->ArgumentCount() == 0 || mask->ArgumentCount() != mask->PhiBlockCount()) return false;
    auto* header = mask->Parent();
    if (condition->Parent() == nullptr || condition->Parent() == header || !MaskDominates(program, condition->Parent(), header)) return false;
    bool entered = false;
    bool repeated = false;
    for (std::size_t index = 0; index < mask->ArgumentCount(); ++index) {
        auto* incoming = mask->Argument(index)->Resolve();
        if (MaskDominates(program, header, mask->PhiBlock(index))) {
            if (!MaskNarrows(incoming, mask, IrOpcode::LogicalAnd, depth - 1, steps) && !Implies(program, incoming, condition, depth - 1, steps)) return false;
            repeated = true;
        } else {
            if (!Implies(program, incoming, condition, depth - 1, steps)) return false;
            entered = true;
        }
    }
    return entered && repeated;
}
}

bool AnyBox::Shader::NativeMaskImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition) {
    if (!mask || !condition || mask->Type() != ShaderRecompiler::IrType::Bool || condition->Type() != ShaderRecompiler::IrType::Bool) return false;
    unsigned steps = 4096;
    return Implies(program, mask, condition, 64, steps);
}
