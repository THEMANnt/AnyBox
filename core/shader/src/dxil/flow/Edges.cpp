#include "Plan.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Plan;

void Plan::splitPhiEdges(const IrProgram& source) {
    for (const auto* block : source.BlockOrder()) {
        std::vector<const IrValue*> phis;
        for (auto* instruction : block->Instructions()) if (instruction->IsPhi()) phis.push_back(instruction);
        if (phis.empty()) continue;
        auto target = ids.at(block);
        for (const auto* predecessor : block->Predecessors()) {
            if (graph.blocks.size() >= UINT32_MAX) throw std::length_error("DXIL PHI edge expansion contains too many blocks");
            Origin origin;
            for (auto* phi : phis) {
                const IrValue* value = nullptr;
                for (std::size_t index = 0; index < phi->PhiBlockCount(); ++index) {
                    if (phi->PhiBlock(index) == predecessor) value = phi->Argument(index);
                }
                if (!value) throw std::logic_error("DXIL PHI edge has no incoming value");
                origin.assignments.push_back({phi, value});
            }
            auto from = ids.at(predecessor);
            BasicBlock edge;
            edge.id = static_cast<std::uint32_t>(graph.blocks.size());
            edge.instructionBegin = edge.startProgramCounter = static_cast<std::uint32_t>(origins.size());
            edge.instructionEnd = edge.endProgramCounter = edge.instructionBegin + 1;
            edge.estimatedSpirvWords = static_cast<std::uint32_t>(origin.assignments.size() * 2);
            edge.terminator.kind = TerminatorKind::Branch;
            edge.terminator.trueBlock = target;
            edge.successors = {target};
            auto& branch = graph.blocks.at(from);
            for (auto& successor : branch.successors) if (successor == target) successor = edge.id;
            if (branch.terminator.trueBlock == target) branch.terminator.trueBlock = edge.id;
            if (branch.terminator.falseBlock == target) branch.terminator.falseBlock = edge.id;
            origins.push_back(std::move(origin));
            graph.blocks.push_back(std::move(edge));
        }
    }
    for (auto& block : graph.blocks) block.predecessors.clear();
    for (const auto& block : graph.blocks) {
        for (auto target : block.successors) graph.blocks.at(target).predecessors.push_back(block.id);
    }
}
