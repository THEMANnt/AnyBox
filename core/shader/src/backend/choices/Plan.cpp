#include "Choice.hpp"
#include <algorithm>
#include <iterator>

std::size_t AnyBox::Shader::PlanDescriptorChoices(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    if (program.Resources().srtPlanComplete || program.Resources().resourceTrackingComplete) throw std::logic_error("Descriptor choices require an unplanned program");
    std::size_t count = 0;
    for (const auto& block : program.Blocks()) {
        std::vector<IrValue*> handles(block->Instructions().begin(), block->Instructions().end());
        for (auto* handle : handles) {
            auto choice = FindDescriptorChoice(program, *handle);
            if (!choice) continue;
            IrBuilder builder(program);
            builder.SetInsertionPoint(*block);
            auto previous = std::prev(block->Instructions().end());
            auto& base = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {choice->reads[0][0].low, choice->reads[0][0].high});
            auto& offset = builder.Select(*choice->condition, builder.Constant(choice->reads[0][0].offset), builder.Constant(choice->reads[0][1].offset));
            for (std::size_t word = 0; word < choice->words; ++word) {
                auto memory = choice->reads[word][0].memory;
                memory.offset = static_cast<std::uint32_t>(word * 4);
                auto flags = choice->reads[word][0].flags;
                flags.index = static_cast<std::uint32_t>(program.Resources().memoryInfo.size());
                program.Resources().memoryInfo.push_back(memory);
                auto& read = builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&base, &offset, &builder.Constant(0), &builder.ConstantBool(true)}, flags);
                handle->ReplaceArgument(word, &read);
            }
            auto insertion = std::find(block->Instructions().begin(), block->Instructions().end(), handle);
            block->Instructions().splice(insertion, block->Instructions(), std::next(previous), block->Instructions().end());
            ++count;
        }
    }
    return count;
}
