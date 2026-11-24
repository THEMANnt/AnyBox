#include "MaskFlow.hpp"
#include <unordered_set>

namespace {
bool SamePhi(const ShaderRecompiler::IrValue* first, const ShaderRecompiler::IrValue* second) {
    if (first == second) return true;
    if (!first->IsPhi() || !second->IsPhi() || first->Parent() != second->Parent() || first->ArgumentCount() != second->ArgumentCount() || second->PhiBlockCount() != second->ArgumentCount()) return false;
    for (std::size_t index = 0; index < first->ArgumentCount(); ++index) {
        bool matched = false;
        for (std::size_t other = 0; other < second->ArgumentCount(); ++other) {
            matched |= first->PhiBlock(index) == second->PhiBlock(other) && first->Argument(index)->Resolve() == second->Argument(other)->Resolve();
        }
        if (!matched) return false;
    }
    return true;
}
}

bool AnyBox::Shader::MaskDominates(const ShaderRecompiler::IrProgram& program, const ShaderRecompiler::IrBlock* first, const ShaderRecompiler::IrBlock* second) {
    std::vector<const ShaderRecompiler::IrBlock*> pending{second};
    std::unordered_set<const ShaderRecompiler::IrBlock*> visited;
    for (std::size_t next = 0; next < pending.size(); ++next) {
        auto* block = pending[next];
        if (block == first || !visited.insert(block).second) continue;
        if (block == program.BlockOrder().front() || block->Predecessors().empty()) return false;
        pending.insert(pending.end(), block->Predecessors().begin(), block->Predecessors().end());
    }
    return true;
}

bool AnyBox::Shader::MaskNarrows(const ShaderRecompiler::IrValue* value, const ShaderRecompiler::IrValue* phi, ShaderRecompiler::IrOpcode operation, unsigned depth, unsigned& steps) {
    if (steps == 0) return false;
    --steps;
    value = value->Resolve();
    if (SamePhi(value, phi)) return true;
    if (depth == 0 || value->Opcode() != operation || value->ArgumentCount() != 2) return false;
    return MaskNarrows(value->Argument(0), phi, operation, depth - 1, steps) || MaskNarrows(value->Argument(1), phi, operation, depth - 1, steps);
}
