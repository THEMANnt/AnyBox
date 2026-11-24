#include "Builder.hpp"
#include <Optimization/SsaBuilder.hpp>
#include <Optimization/ConstantFolder.hpp>
#include <Optimization/DeadCodeEliminator.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Builder;

Builder::Builder(IrProgram& source) : source(source), plan(source) {}

IrProgram Builder::Run() {
    if (source.Resources().srtPlanComplete || source.Resources().resourceTrackingComplete || source.Metadata().shaderInfoComplete || source.Metadata().bindingLayoutComplete || !source.Metadata().dynamicReads.empty()) throw std::logic_error("DXIL structurization requires an unplanned function");
    output.Resources() = std::move(source.Resources());
    output.Metadata() = source.Metadata();
    output.Metadata().blockInfo.clear();
    output.SetWaveSize(source.WaveSize());
    createBlocks();
    createInstructions();
    connectInstructions();
    connectConditions();
    SsaBuilder{}.Rewrite(output);
    ConstantFolder{}.Fold(output);
    ResolveControlFlowIdentities(output);
    DeadCodeEliminator{}.RemoveIdentities(output);
    DeadCodeEliminator{}.Eliminate(output);
    ValidateProgram(output, true);
    return std::move(output);
}

void Builder::createBlocks() {
    states.resize(plan.graph.blocks.size());
    for (const auto& node : plan.graph.blocks) {
        auto& state = states.at(node.id);
        state.block = &output.CreateBlock();
        output.BlockOrder().push_back(state.block);
        ids.emplace(state.block, node.id);
        if (node.instructionBegin != node.instructionEnd) {
            if (node.instructionEnd != node.instructionBegin + 1) throw std::logic_error("DXIL structured block combines unrelated origins");
            state.origin = &plan.origins.at(node.instructionBegin);
        }
        BlockInfo info;
        info.id = node.id;
        info.startPc = node.startProgramCounter;
        info.endPc = node.endProgramCounter;
        info.terminator = node.terminator;
        output.Metadata().blockInfo.push_back(std::move(info));
    }
    output.SetEntryBlock(*states.at(plan.graph.entryBlock).block);
    for (const auto& node : plan.graph.blocks) {
        for (auto successor : node.successors) states.at(node.id).block->AddBranch(states.at(successor).block);
    }
}

void Builder::createInstructions() {
    for (auto& state : states) {
        if (!state.origin) continue;
        if (state.origin->block) {
            for (auto* instruction : state.origin->block->Instructions()) {
                auto opcode = instruction->Opcode();
                if (instruction->IsPhi() || opcode == IrOpcode::Branch || opcode == IrOpcode::BranchConditional || opcode == IrOpcode::Return) continue;
                auto& value = output.CreateValue(opcode, instruction->Type(), instruction->Flags<std::uint64_t>());
                state.block->AppendInstruction(&value);
                state.instructions.emplace_back(instruction, &value);
                if (instruction->Type() != IrType::Void) state.definitions.emplace(instruction, &value);
            }
        }
        for (const auto& assignment : state.origin->assignments) {
            auto& value = output.CreateValue(IrOpcode::Phi, assignment.target->Type());
            state.block->AppendInstruction(&value);
            state.definitions.emplace(assignment.target, &value);
        }
    }
}
