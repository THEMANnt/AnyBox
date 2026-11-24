#include "Slices.hpp"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

void AnyBox::Shader::JoinNativeSlices(ShaderRecompiler::IrProgram& program, const NativeSlices& slices, const ShaderRecompiler::ControlFlowGraph& original) {
    using namespace ShaderRecompiler;
    auto oldOrder = program.BlockOrder();
    auto oldInfo = program.Metadata().blockInfo;
    std::unordered_map<std::uint32_t, std::size_t> indices;
    for (std::size_t index = 0; index < oldInfo.size(); ++index) indices.emplace(oldInfo[index].id, index);
    std::vector<IrBlock*> order{oldOrder.front()};
    std::vector<BlockInfo> info{oldInfo.front()};
    std::unordered_map<std::uint32_t, IrBlock*> blocks;
    for (std::size_t index = 0; index < slices.groups.size(); ++index) {
        const auto& group = slices.groups[index];
        auto* first = oldOrder.at(indices.at(group.front()));
        for (std::size_t part = 1; part < group.size(); ++part) {
            auto* source = oldOrder.at(indices.at(group[part]));
            for (auto* instruction : source->Instructions()) instruction->SetParent(first);
            first->Instructions().splice(first->Instructions().end(), source->Instructions());
        }
        auto metadata = oldInfo.at(indices.at(group.back()));
        const auto& source = original.blocks.at(index);
        metadata.id = source.id;
        metadata.startPc = source.startProgramCounter;
        metadata.endPc = source.endProgramCounter;
        blocks.emplace(source.id, first);
        order.push_back(first);
        info.push_back(std::move(metadata));
    }
    for (auto* block : order) {
        block->Predecessors().clear();
        block->Successors().clear();
    }
    for (const auto& source : original.blocks) {
        for (auto successor : source.successors) blocks.at(source.id)->AddBranch(blocks.at(successor));
    }
    order.front()->AddBranch(blocks.at(original.blocks.front().id));
    std::unordered_set<IrBlock*> retained(order.begin(), order.end());
    std::erase_if(program.Blocks(), [&](const auto& block) { return !retained.contains(block.get()); });
    program.BlockOrder() = std::move(order);
    program.Metadata().blockInfo = std::move(info);
    ValidateProgram(program, false);
}
