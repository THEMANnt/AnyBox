#include "Slices.hpp"
#include <Translation/TranslationContext.hpp>
#include <algorithm>
#include <stdexcept>

ShaderRecompiler::IrProgram AnyBox::Shader::TranslateNativeProgram(const ShaderRecompiler::RdnaProgram& decoded, const ShaderRecompiler::ControlFlowGraph& graph, const ShaderRecompiler::TranslateOptions& options) {
    using namespace ShaderRecompiler;
    if (std::ranges::none_of(decoded.instructions, IsNativeExtension)) return InstructionTranslator{}.Translate(decoded, graph, options);
    auto slices = SliceNativeExtensions(decoded, graph);
    auto skeleton = decoded;
    for (auto& instruction : skeleton.instructions) {
        if (!IsNativeExtension(instruction)) continue;
        instruction.op = RdnaOpcode::SNop;
        instruction.family = RdnaInstructionFamily::SOPP;
    }
    auto program = InstructionTranslator{}.Translate(skeleton, slices.graph, options);
    auto windows = NativeWindowModes(decoded, slices.graph, program.Metadata().vectorWindowIndexedWrites, program.Metadata().vectorWindowSize != 0);
    auto rounding = ComputeEntryRounding(slices.graph, decoded, options.floatMode.has_value() ? options.floatMode->floatMode : ModeUndeclaredSeed);
    for (std::size_t index = 0; index < slices.graph.blocks.size(); ++index) {
        const auto& block = slices.graph.blocks[index];
        if (block.instructionEnd != block.instructionBegin + 1) continue;
        const auto& instruction = decoded.instructions.at(block.instructionBegin);
        if (!IsNativeExtension(instruction)) continue;
        auto& irBlock = *program.BlockOrder().at(index + 1);
        auto& metadata = program.Metadata().blockInfo.at(index + 1);
        metadata.condition = nullptr;
        metadata.indirectTarget = nullptr;
        while (!irBlock.Instructions().empty()) {
            auto* value = irBlock.Instructions().back();
            irBlock.RemoveInstruction(value);
            value->Invalidate();
        }
        EmitNativeLegacyFma(program, irBlock, instruction, options, rounding.at(block.id), windows.at(index));
        TranslationContext context(program, irBlock, NumVectorRegs);
        context.SetVectorWindowActive(windows[index].active);
        if (windows[index].synchronizeAfter) context.SyncVectorWindow(!windows[index].active);
        context.AddBranchCondition(block, metadata);
    }
    JoinNativeSlices(program, slices, graph);
    return program;
}
