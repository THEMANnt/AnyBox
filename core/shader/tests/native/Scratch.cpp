#include "../../src/native/ScratchWrites.hpp"
#include "NativeFixtures.hpp"
#include <array>

void CheckNativeScratchWrites() {
    using namespace Fixtures;
    using namespace ShaderRecompiler;
    using AnyBox::Shader::RemoveUnusedScratchWrites;
    RdnaProgram program;
    RdnaInstruction instruction;
    instruction.op = RdnaOpcode::SSetregImm32B32;
    instruction.programCounter = 4;
    instruction.wordCount = 2;
    instruction.rawWords[0] = 0xBA80F814;
    program.instructions.push_back(instruction);
    auto changed = RemoveUnusedScratchWrites(program);
    Require(changed && changed->instructions.size() == 1);
    Require(changed->instructions[0].op == RdnaOpcode::SNop && program.instructions[0].op == RdnaOpcode::SSetregImm32B32);
    Require(changed->instructions[0].programCounter == 4 && changed->instructions[0].wordCount == 2);
    for (unsigned segment : {0u, 1u, 3u}) {
        auto observed = program;
        RdnaInstruction memory;
        memory.family = RdnaInstructionFamily::FLAT;
        memory.memorySegment = segment;
        observed.instructions.push_back(memory);
        Require(!RemoveUnusedScratchWrites(observed));
    }
    for (auto opcode : {RdnaOpcode::SGetregB32, RdnaOpcode::SSwappcB64, RdnaOpcode::SSetpcB64, RdnaOpcode::SSendmsg, RdnaOpcode::SSendmsghalt}) {
        auto observed = program;
        instruction.op = opcode;
        observed.instructions.push_back(instruction);
        Require(!RemoveUnusedScratchWrites(observed));
    }
    for (unsigned identifier : {20u, 21u}) {
        for (auto opcode : {RdnaOpcode::SSetregB32, RdnaOpcode::SSetregImm32B32}) {
            program.instructions[0].op = opcode;
            program.instructions[0].rawWords[0] = 0xBA80F800 | identifier;
            Require(RemoveUnusedScratchWrites(program).has_value());
        }
    }
    program.instructions[0].rawWords[0] = 0xBA80F801;
    Require(!RemoveUnusedScratchWrites(program));
    program.instructions[0].rawWords[0] = 0xBA80F854;
    Reject<std::invalid_argument>([&] { (void)RemoveUnusedScratchWrites(program); });
}
