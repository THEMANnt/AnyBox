#include "Slices.hpp"
#include <algorithm>
#include <stdexcept>

bool AnyBox::Shader::IsNativeExtension(const ShaderRecompiler::RdnaInstruction& instruction) {
    return instruction.family == ShaderRecompiler::RdnaInstructionFamily::VOP3 && instruction.opcodeId == 0x140;
}

AnyBox::Shader::NativeSlices AnyBox::Shader::SliceNativeExtensions(const ShaderRecompiler::RdnaProgram& program, const ShaderRecompiler::ControlFlowGraph& graph) {
    using namespace ShaderRecompiler;
    NativeSlices result{graph, {}};
    result.graph.blocks.clear();
    std::uint32_t next = 0;
    for (const auto& block : graph.blocks) next = std::max(next, block.id);
    for (const auto& source : graph.blocks) {
        std::vector<std::uint32_t> boundaries{source.instructionBegin};
        for (auto index = source.instructionBegin; index < source.instructionEnd; ++index) {
            if (IsNativeExtension(program.instructions.at(index))) {
                if (boundaries.back() != index) boundaries.push_back(index);
                boundaries.push_back(index + 1);
            }
        }
        if (boundaries.back() != source.instructionEnd) boundaries.push_back(source.instructionEnd);
        if (boundaries.size() == 1) boundaries.push_back(boundaries.front());
        std::vector<std::uint32_t> group{source.id};
        for (std::size_t index = 2; index < boundaries.size(); ++index) {
            if (next >= InvalidControlFlowId - 1) throw std::length_error("Native instruction slices exhaust control flow identifiers");
            group.push_back(++next);
        }
        for (std::size_t part = 0; part < group.size(); ++part) {
            auto block = source;
            block.id = group[part];
            block.instructionBegin = boundaries[part];
            block.instructionEnd = boundaries[part + 1];
            block.startProgramCounter = part == 0 ? source.startProgramCounter : program.instructions.at(boundaries[part]).programCounter;
            block.endProgramCounter = part + 1 == group.size() ? source.endProgramCounter : program.instructions.at(boundaries[part + 1]).programCounter;
            if (part + 1 != group.size()) {
                block.terminator = {};
                block.terminator.kind = TerminatorKind::Branch;
                block.terminator.trueBlock = group[part + 1];
                block.successors = {group[part + 1]};
            }
            block.predecessors.clear();
            result.graph.blocks.push_back(std::move(block));
        }
        result.groups.push_back(std::move(group));
    }
    for (const auto& block : result.graph.blocks) {
        for (auto successor : block.successors) result.graph.FindBlock(successor).predecessors.push_back(block.id);
    }
    return result;
}
