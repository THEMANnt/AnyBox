#include "Plan.hpp"
#include <ControlFlow/Structurizer.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Plan;

Plan::Plan(const IrProgram& source) {
    ValidateProgram(source, true);
    const auto& blocks = source.BlockOrder();
    if (blocks.size() >= UINT32_MAX) throw std::length_error("DXIL control flow contains too many blocks");
    std::unordered_map<std::uint32_t, std::uint32_t> targets;
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        auto id = static_cast<std::uint32_t>(index);
        ids.emplace(blocks[index], id);
        if (!targets.emplace(source.Metadata().blockInfo[index].id, id).second) throw std::logic_error("DXIL control flow repeats a block identifier");
    }
    for (std::size_t index = 0; index < blocks.size(); ++index) {
        auto* block = blocks[index];
        const auto& info = source.Metadata().blockInfo[index];
        if (info.terminator.kind != TerminatorKind::Branch && info.terminator.kind != TerminatorKind::ConditionalBranch && info.terminator.kind != TerminatorKind::Return) throw std::runtime_error("DXIL control flow terminator is not implemented");
        BasicBlock node;
        node.id = static_cast<std::uint32_t>(index);
        node.instructionBegin = node.startProgramCounter = node.id;
        node.instructionEnd = node.endProgramCounter = node.id + 1;
        if (block->Instructions().size() > UINT32_MAX / 8) throw std::length_error("DXIL control flow block exceeds the instruction estimate limit");
        node.estimatedSpirvWords = static_cast<std::uint32_t>(block->Instructions().size() * 8);
        node.terminator = info.terminator;
        if (node.terminator.kind != TerminatorKind::Return) node.terminator.trueBlock = targets.at(info.terminator.trueBlock);
        if (node.terminator.kind == TerminatorKind::ConditionalBranch) node.terminator.falseBlock = targets.at(info.terminator.falseBlock);
        node.terminator.mergeBlock = node.terminator.continueBlock = InvalidControlFlowId;
        node.terminator.loopHeader = false;
        for (auto* successor : block->Successors()) node.successors.push_back(ids.at(successor));
        for (auto* predecessor : block->Predecessors()) node.predecessors.push_back(ids.at(predecessor));
        origins.push_back({block, info.condition, {}});
        graph.blocks.push_back(std::move(node));
    }
    graph.entryBlock = ids.at(&source.EntryBlock());
    splitPhiEdges(source);
    Structurizer{}.Structurize(graph);
    if (graph.unsupported || graph.irreducible) throw std::runtime_error("DXIL control flow structurization failed: " + graph.unsupportedReason);
}
