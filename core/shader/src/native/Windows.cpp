#include "Slices.hpp"
#include <algorithm>
#include <cstdlib>
#include <unordered_map>

namespace {
std::vector<bool> Reach(const std::vector<std::vector<std::size_t>>& edges, const std::vector<std::size_t>& seeds) {
    std::vector<bool> reached(edges.size());
    auto pending = seeds;
    for (auto seed : seeds) reached.at(seed) = true;
    for (std::size_t next = 0; next < pending.size(); ++next) {
        for (auto target : edges.at(pending[next])) {
            if (reached.at(target)) continue;
            reached[target] = true;
            pending.push_back(target);
        }
    }
    return reached;
}

bool Relative(ShaderRecompiler::RdnaOpcode opcode) {
    using enum ShaderRecompiler::RdnaOpcode;
    return opcode == VMovrelsB32 || opcode == VMovreldB32 || opcode == VMovrelsdB32 || opcode == VMovrelsd2B32 || opcode == VSwaprelB32;
}
}

std::vector<AnyBox::Shader::NativeWindowMode> AnyBox::Shader::NativeWindowModes(const ShaderRecompiler::RdnaProgram& program, const ShaderRecompiler::ControlFlowGraph& graph, bool indexedWrites, bool enabled) {
    std::vector<NativeWindowMode> result(graph.blocks.size());
    if (!enabled) return result;
    std::unordered_map<std::uint32_t, std::size_t> indices;
    for (std::size_t index = 0; index < graph.blocks.size(); ++index) indices.emplace(graph.blocks[index].id, index);
    std::vector<std::vector<std::size_t>> successors(graph.blocks.size()), predecessors(graph.blocks.size());
    std::vector<std::size_t> relative;
    for (std::size_t index = 0; index < graph.blocks.size(); ++index) {
        const auto& block = graph.blocks[index];
        for (auto successor : block.successors) {
            auto target = indices.at(successor);
            successors[index].push_back(target);
            predecessors[target].push_back(index);
        }
        for (auto instruction = block.instructionBegin; instruction < block.instructionEnd; ++instruction) {
            if (!Relative(program.instructions.at(instruction).op)) continue;
            relative.push_back(index);
            break;
        }
    }
    auto forward = Reach(successors, relative);
    auto backward = Reach(predecessors, relative);
    std::size_t size = 0;
    for (std::size_t index = 0; index < result.size(); ++index) {
        result[index].active = forward[index] && backward[index];
        size += result[index].active;
    }
    static const bool unscoped = std::getenv("APS5_NO_SCOPED_VECTOR_WINDOW") != nullptr;
    bool scoped = !unscoped && size != 0 && size != result.size();
    if (!scoped) {
        for (auto& mode : result) mode.active = true;
        return result;
    }
    auto boundary = [&](std::size_t index) {
        return std::ranges::none_of(predecessors[index], [&](auto predecessor) { return result[index].active == result[predecessor].active; });
    };
    for (std::size_t index = 0; index < result.size(); ++index) {
        auto& mode = result[index];
        mode.synchronizeBefore = (mode.active || indexedWrites) && boundary(index);
        mode.synchronizeAfter = (!mode.active || indexedWrites) && std::ranges::any_of(successors[index], [&](auto target) { return mode.active != result[target].active && !boundary(target); });
    }
    return result;
}
