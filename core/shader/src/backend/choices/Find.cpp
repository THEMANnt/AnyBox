#include "Choice.hpp"
#include <Optimization/SrtWalker/SrtRuntimeValidator.hpp>
#include <algorithm>
#include <unordered_set>

namespace {
using namespace ShaderRecompiler;

bool Quiet(const IrBlock& block, const IrValue* end = nullptr) {
    for (const auto* value : block.Instructions()) {
        if (value == end) break;
        if (value->MayHaveSideEffects() && value->Opcode() != IrOpcode::Waitcnt) return false;
    }
    return true;
}

bool Available(IrProgram& program, IrValue* value, IrBlock* controller) {
    auto* definition = value->Parent();
    if (definition == nullptr) return true;
    std::vector<IrBlock*> pending{controller};
    std::unordered_set<IrBlock*> visited;
    for (std::size_t next = 0; next < pending.size(); ++next) {
        auto* block = pending[next];
        if (block == definition || !visited.insert(block).second) continue;
        if (block == program.BlockOrder().front() || block->Predecessors().empty()) return false;
        pending.insert(pending.end(), block->Predecessors().begin(), block->Predecessors().end());
    }
    return true;
}
}

std::optional<AnyBox::Shader::DescriptorChoice> AnyBox::Shader::FindDescriptorChoice(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& handle) {
    using namespace ShaderRecompiler;
    auto width = handle.Opcode() == IrOpcode::GetImageResource ? 8u : handle.Opcode() == IrOpcode::GetBufferResource || handle.Opcode() == IrOpcode::GetSamplerResource ? 4u : 0u;
    auto* join = handle.Parent();
    if (width == 0 || handle.ArgumentCount() != width || join->Predecessors().size() != 2 || !Quiet(*join, &handle)) return std::nullopt;
    auto* first = join->Predecessors()[0];
    auto* second = join->Predecessors()[1];
    if (first == second || first->Predecessors().size() != 1 || second->Predecessors().size() != 1) return std::nullopt;
    auto* controller = first->Predecessors().front();
    if (controller != second->Predecessors().front() || controller->Successors().size() != 2) return std::nullopt;
    if (first->Successors().size() != 1 || second->Successors().size() != 1 || !Quiet(*first) || !Quiet(*second)) return std::nullopt;
    auto position = std::ranges::find(program.BlockOrder(), controller);
    const auto& metadata = program.Metadata().blockInfo.at(static_cast<std::size_t>(position - program.BlockOrder().begin()));
    if (metadata.terminator.kind != TerminatorKind::ConditionalBranch || metadata.condition == nullptr) return std::nullopt;
    if (!Detail::RuntimeValidator(program.Resources(), RuntimeValueType::Any).Run(metadata.condition)) return std::nullopt;
    auto firstPosition = std::ranges::find(program.BlockOrder(), first);
    auto firstId = program.Metadata().blockInfo.at(static_cast<std::size_t>(firstPosition - program.BlockOrder().begin())).id;
    auto* yes = firstId == metadata.terminator.trueBlock ? first : second;
    auto* no = yes == first ? second : first;
    DescriptorChoice choice;
    choice.condition = metadata.condition;
    choice.words = width;
    for (unsigned word = 0; word < width; ++word) {
        auto* phi = handle.Argument(word)->Resolve();
        if (!phi->IsPhi() || phi->Parent() != join || phi->PhiBlockCount() != 2 || phi->ArgumentCount() != 2 || phi->UseCount() != 1) return std::nullopt;
        for (unsigned arm = 0; arm < 2; ++arm) {
            auto* predecessor = arm == 0 ? yes : no;
            unsigned operand = phi->PhiBlock(0) == predecessor ? 0 : phi->PhiBlock(1) == predecessor ? 1 : 2;
            if (operand == 2) return std::nullopt;
            auto* value = phi->Argument(operand)->Resolve();
            if (value->Parent() != predecessor) return std::nullopt;
            auto read = ReadDescriptor(program, *value);
            if (!read || !Available(program, read->low, controller) || !Available(program, read->high, controller)) return std::nullopt;
            const auto& shared = choice.reads[0][0];
            if (shared.low != nullptr && (!SameDescriptorValue(read->low, shared.low) || !SameDescriptorValue(read->high, shared.high))) return std::nullopt;
            if (word != 0 && static_cast<std::uint64_t>(choice.reads[0][arm].offset) + word * 4 != read->offset) return std::nullopt;
            choice.reads[word][arm] = *read;
        }
        auto yesMemory = choice.reads[word][0].memory;
        auto noMemory = choice.reads[word][1].memory;
        yesMemory.offset = noMemory.offset = 0;
        if (yesMemory != noMemory) return std::nullopt;
    }
    return choice;
}
