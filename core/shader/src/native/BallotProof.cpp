#include "BitProof.hpp"
#include "MaskFlow.hpp"

namespace {
using namespace ShaderRecompiler;
using namespace AnyBox::Shader;

bool Constant(const IrValue* value, std::uint32_t expected) {
    value = value->Resolve();
    return value->Type() == IrType::U32 && value->HasImmediate() && value->ImmediateU32() == expected;
}

bool WordImplies(const IrProgram& program, const IrValue* word, const IrValue* condition, unsigned component, unsigned depth, unsigned& steps) {
    if (steps == 0 || depth == 0) return false;
    --steps;
    word = word->Resolve();
    if (Constant(word, 0)) return true;
    if (word->Opcode() == IrOpcode::BitwiseAnd32 && word->ArgumentCount() == 2) return WordImplies(program, word->Argument(0), condition, component, depth - 1, steps) || WordImplies(program, word->Argument(1), condition, component, depth - 1, steps);
    if (word->Opcode() == IrOpcode::BitwiseOr32 && word->ArgumentCount() == 2) return WordImplies(program, word->Argument(0), condition, component, depth - 1, steps) && WordImplies(program, word->Argument(1), condition, component, depth - 1, steps);
    if (word->Opcode() == IrOpcode::CompositeExtractU32x4 && word->ArgumentCount() == 2 && Constant(word->Argument(1), component)) {
        auto* ballot = word->Argument(0)->Resolve();
        return ballot->Opcode() == IrOpcode::Ballot && ballot->ArgumentCount() == 1 && ballot->Argument(0)->Resolve() == condition->Resolve();
    }
    if (!word->IsPhi() || word->ArgumentCount() == 0 || word->ArgumentCount() != word->PhiBlockCount()) return false;
    auto* header = word->Parent();
    if (condition->Parent() == nullptr || condition->Parent() == header || !MaskDominates(program, condition->Parent(), header)) return false;
    bool entered = false;
    bool repeated = false;
    for (std::size_t index = 0; index < word->ArgumentCount(); ++index) {
        auto* incoming = word->Argument(index)->Resolve();
        if (MaskDominates(program, header, word->PhiBlock(index))) {
            if (!MaskNarrows(incoming, word, IrOpcode::BitwiseAnd32, depth - 1, steps)) return false;
            repeated = true;
        } else {
            if (!WordImplies(program, incoming, condition, component, depth - 1, steps)) return false;
            entered = true;
        }
    }
    return entered && repeated;
}

}

bool AnyBox::Shader::NativeBallotPredicateImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition, unsigned& steps) {
    auto source = ReadNativeBitPredicate(mask);
    if (!source) return false;
    auto selector = ReadNativeLaneMask(source->selector);
    if (!selector) return false;
    if (program.WaveSize() == 32 && (*selector == 31 || *selector == UINT32_MAX)) return WordImplies(program, source->word, condition, 0, 64, steps);
    if (program.WaveSize() != 64 || *selector != 31 || source->word->Opcode() != ShaderRecompiler::IrOpcode::SelectU32 || source->word->ArgumentCount() != 3 || !NativeLowHalf(source->word->Argument(0))) return false;
    return WordImplies(program, source->word->Argument(1), condition, 0, 64, steps) && WordImplies(program, source->word->Argument(2), condition, 1, 64, steps);
}
