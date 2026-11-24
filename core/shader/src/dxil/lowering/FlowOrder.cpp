#include "Function.hpp"
#include <algorithm>
#include <unordered_set>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::FunctionLowering;

void FunctionLowering::orderBlocks() {
    auto& blockOrder = program.BlockOrder();
    auto& metadata = program.Metadata().blockInfo;
    if (blockOrder.size() != metadata.size()) throw std::logic_error("DXIL block order and metadata have different sizes");
    std::unordered_map<const IrBlock*, std::size_t> positions;
    for (std::size_t index = 0; index < blockOrder.size(); ++index) {
        if (!blockOrder[index] || !positions.emplace(blockOrder[index], index).second) throw std::logic_error("DXIL block order contains a null or repeated block");
    }
    std::unordered_set<const IrBlock*> visited;
    std::vector<IrBlock*> ordered;
    ordered.reserve(blockOrder.size());
    std::vector<std::pair<IrBlock*, bool>> pending{{&program.EntryBlock(), false}};
    while (!pending.empty()) {
        auto [block, complete] = pending.back();
        pending.pop_back();
        if (!positions.contains(block)) throw std::logic_error("DXIL lowered branch targets a foreign block");
        if (complete) { ordered.push_back(block); continue; }
        if (!visited.insert(block).second) continue;
        pending.emplace_back(block, true);
        for (auto* successor : block->Successors()) pending.emplace_back(successor, false);
    }
    if (ordered.size() != blockOrder.size()) throw std::runtime_error("DXIL lowered function contains unreachable blocks");
    std::ranges::reverse(ordered);
    std::vector<BlockInfo> reordered;
    std::vector<std::unique_ptr<IrBlock>> storage;
    reordered.reserve(metadata.size());
    storage.reserve(ordered.size());
    for (auto* block : ordered) {
        auto position = positions.at(block);
        reordered.push_back(std::move(metadata.at(position)));
        storage.push_back(std::move(program.Blocks().at(position)));
    }
    metadata = std::move(reordered);
    program.Blocks() = std::move(storage);
    blockOrder = std::move(ordered);
}
