#include "Decode.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <stdexcept>
#include <vector>

ShaderRecompiler::RdnaProgram AnyBox::Shader::DecodeNativeProgram(std::span<const std::uint32_t> code) {
    using namespace ShaderRecompiler;
    RdnaProgram program;
    std::vector<std::uint32_t> normalized;
    for (;;) {
        auto input = normalized.empty() ? code : std::span<const std::uint32_t>(normalized);
        try {
            DecodeRdnaProgram(input, program);
            break;
        } catch (const std::invalid_argument&) {
            std::size_t next = 0;
            if (!program.instructions.empty()) {
                const auto& last = program.instructions.back();
                if (last.op == RdnaOpcode::SBranch || last.op == RdnaOpcode::SEndpgm || last.op == RdnaOpcode::SCodeEnd) throw;
                next = static_cast<std::size_t>(last.programCounter / 4) + last.wordCount;
            }
            if (next >= input.size() || input.size() - next < 2 || (input[next] & 0xFFFF0000) != 0xD5400000) throw;
            if (normalized.empty()) normalized.assign(code.begin(), code.end());
            normalized[next] = (normalized[next] & 0x0000FFFF) | 0xD54B0000;
        }
    }
    program.code = code;
    for (auto& instruction : program.instructions) {
        auto index = instruction.programCounter / 4;
        if (instruction.family != RdnaInstructionFamily::VOP3 || (code[index] & 0xFFFF0000) != 0xD5400000) continue;
        instruction.op = RdnaOpcode::VMadLegacyF32;
        instruction.opcodeId = 0x140;
        SetRdnaRawWords(instruction, code, index, instruction.wordCount);
    }
    return program;
}
