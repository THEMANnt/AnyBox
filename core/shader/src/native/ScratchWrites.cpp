#include "ScratchWrites.hpp"
#include <stdexcept>

namespace {
bool ScratchRegister(const ShaderRecompiler::RdnaInstruction& instruction) {
    auto identifier = instruction.rawWords[0] & 63;
    return identifier == 20 || identifier == 21;
}
}

std::optional<ShaderRecompiler::RdnaProgram> AnyBox::Shader::RemoveUnusedScratchWrites(const ShaderRecompiler::RdnaProgram& program) {
    using namespace ShaderRecompiler;
    bool written = false;
    bool observed = false;
    for (const auto& instruction : program.instructions) {
        if (instruction.family == RdnaInstructionFamily::FLAT && instruction.memorySegment != 2) observed = true;
        if (instruction.op == RdnaOpcode::SSendmsg || instruction.op == RdnaOpcode::SSendmsghalt || instruction.op == RdnaOpcode::SSwappcB64 || instruction.op == RdnaOpcode::SSetpcB64) observed = true;
        if (instruction.op == RdnaOpcode::SGetregB32 && ScratchRegister(instruction)) observed = true;
        if ((instruction.op == RdnaOpcode::SSetregB32 || instruction.op == RdnaOpcode::SSetregImm32B32) && ScratchRegister(instruction)) {
            auto offset = (instruction.rawWords[0] >> 6) & 31;
            auto width = ((instruction.rawWords[0] >> 11) & 31) + 1;
            if (offset + width > 32) throw std::invalid_argument("Native scratch register write exceeds the hardware register width");
            written = true;
        }
    }
    if (!written || observed) return std::nullopt;
    auto result = program;
    for (auto& instruction : result.instructions) {
        if ((instruction.op == RdnaOpcode::SSetregB32 || instruction.op == RdnaOpcode::SSetregImm32B32) && ScratchRegister(instruction)) instruction.op = RdnaOpcode::SNop;
    }
    return result;
}
