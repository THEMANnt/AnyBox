#include "FirstLane.hpp"

std::size_t AnyBox::Shader::SimplifyFirstLaneGuards(ShaderRecompiler::IrProgram& program) {
    using namespace ShaderRecompiler;
    std::size_t removed = 0;
    for (const auto& block : program.Blocks()) {
        for (auto* instruction : block->Instructions()) {
            if (instruction->Opcode() != IrOpcode::ReadFirstLane || instruction->ArgumentCount() != 2) continue;
            for (;;) {
                auto* value = instruction->Argument(0)->Resolve();
                if (value->Opcode() != IrOpcode::SelectU32 || value->ArgumentCount() != 3 || !NativeMaskImplies(program, instruction->Argument(1), value->Argument(0))) break;
                instruction->ReplaceArgument(0, value->Argument(1));
                ++removed;
            }
        }
    }
    return removed;
}
