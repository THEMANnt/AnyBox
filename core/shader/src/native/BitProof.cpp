#include "BitProof.hpp"
#include "MaskFlow.hpp"
#include <optional>

namespace {
using namespace ShaderRecompiler;

bool Constant(const IrValue* value, std::uint32_t expected) {
    value = value->Resolve();
    return value->Type() == IrType::U32 && value->HasImmediate() && value->ImmediateU32() == expected;
}

}

std::optional<AnyBox::Shader::NativeBitPredicate> AnyBox::Shader::ReadNativeBitPredicate(const ShaderRecompiler::IrValue* value) {
    using namespace ShaderRecompiler;
    value = value->Resolve();
    if (value->Opcode() != IrOpcode::INotEqual32 || value->ArgumentCount() != 2) return std::nullopt;
    auto* bit = Constant(value->Argument(1), 0) ? value->Argument(0)->Resolve() : Constant(value->Argument(0), 0) ? value->Argument(1)->Resolve() : nullptr;
    if (!bit || bit->Opcode() != IrOpcode::BitwiseAnd32 || bit->ArgumentCount() != 2) return std::nullopt;
    auto* shifted = Constant(bit->Argument(1), 1) ? bit->Argument(0)->Resolve() : Constant(bit->Argument(0), 1) ? bit->Argument(1)->Resolve() : nullptr;
    if (!shifted || shifted->Opcode() != IrOpcode::ShiftRightLogical32 || shifted->ArgumentCount() != 2) return std::nullopt;
    return NativeBitPredicate{shifted->Argument(0)->Resolve(), shifted->Argument(1)->Resolve()};
}

std::optional<std::uint32_t> AnyBox::Shader::ReadNativeLaneMask(const ShaderRecompiler::IrValue* value) {
    using namespace ShaderRecompiler;
    value = value->Resolve();
    if (value->Opcode() == IrOpcode::LaneId) return UINT32_MAX;
    if (value->Opcode() != IrOpcode::BitwiseAnd32 || value->ArgumentCount() != 2) return std::nullopt;
    for (unsigned index = 0; index < 2; ++index) {
        auto* lane = value->Argument(index)->Resolve();
        auto* mask = value->Argument(1 - index)->Resolve();
        if (lane->Opcode() == IrOpcode::LaneId && mask->Type() == IrType::U32 && mask->HasImmediate()) return mask->ImmediateU32();
    }
    return std::nullopt;
}

bool AnyBox::Shader::NativeLowHalf(const ShaderRecompiler::IrValue* value) {
    using namespace ShaderRecompiler;
    value = value->Resolve();
    return value->Opcode() == IrOpcode::ULessThan32 && value->ArgumentCount() == 2 && value->Argument(0)->Resolve()->Opcode() == IrOpcode::LaneId && Constant(value->Argument(1), 32);
}

namespace {
using namespace ShaderRecompiler;
using namespace AnyBox::Shader;

bool Subset(const IrProgram& program, const IrValue* value, const IrValue* limit, unsigned depth, unsigned& steps) {
    if (steps == 0) return false;
    --steps;
    value = value->Resolve();
    limit = limit->Resolve();
    if (value == limit || Constant(value, 0) || Constant(limit, UINT32_MAX)) return true;
    if (value->Type() == IrType::U32 && limit->Type() == IrType::U32 && value->HasImmediate() && limit->HasImmediate()) return (value->ImmediateU32() & ~limit->ImmediateU32()) == 0;
    if (depth == 0) return false;
    if (value->Opcode() == IrOpcode::BitwiseAnd32 && value->ArgumentCount() == 2) return Subset(program, value->Argument(0), limit, depth - 1, steps) || Subset(program, value->Argument(1), limit, depth - 1, steps);
    if (value->Opcode() == IrOpcode::BitwiseOr32 && value->ArgumentCount() == 2) return Subset(program, value->Argument(0), limit, depth - 1, steps) && Subset(program, value->Argument(1), limit, depth - 1, steps);
    if (value->Opcode() == IrOpcode::SelectU32 && value->ArgumentCount() == 3 && limit->Opcode() == IrOpcode::SelectU32 && limit->ArgumentCount() == 3) {
        bool same = value->Argument(0)->Resolve() == limit->Argument(0)->Resolve() || (NativeLowHalf(value->Argument(0)) && NativeLowHalf(limit->Argument(0)));
        if (same) return Subset(program, value->Argument(1), limit->Argument(1), depth - 1, steps) && Subset(program, value->Argument(2), limit->Argument(2), depth - 1, steps);
    }
    if (!value->IsPhi() || value->ArgumentCount() == 0 || value->ArgumentCount() != value->PhiBlockCount()) return false;
    auto* header = value->Parent();
    if (!limit->HasImmediate() && (limit->Parent() == nullptr || limit->Parent() == header || !MaskDominates(program, limit->Parent(), header))) return false;
    bool entered = false;
    bool repeated = false;
    for (std::size_t index = 0; index < value->ArgumentCount(); ++index) {
        auto* incoming = value->Argument(index)->Resolve();
        if (MaskDominates(program, header, value->PhiBlock(index))) {
            if (!MaskNarrows(incoming, value, IrOpcode::BitwiseAnd32, depth - 1, steps)) return false;
            repeated = true;
        } else {
            if (!Subset(program, incoming, limit, depth - 1, steps)) return false;
            entered = true;
        }
    }
    return entered && repeated;
}

}

bool AnyBox::Shader::NativeBitPredicateImplies(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrValue* mask, const ShaderRecompiler::IrValue* condition, unsigned& steps) {
    auto source = ReadNativeBitPredicate(mask);
    auto target = ReadNativeBitPredicate(condition);
    if (!source || !target) return false;
    if (source->selector != target->selector) {
        auto sourceMask = ReadNativeLaneMask(source->selector);
        auto targetMask = ReadNativeLaneMask(target->selector);
        if (!sourceMask || !targetMask || *sourceMask != *targetMask) return false;
    }
    return Subset(program, source->word, target->word, 64, steps);
}
