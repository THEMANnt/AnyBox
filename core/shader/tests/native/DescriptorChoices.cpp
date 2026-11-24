#include "../../src/backend/choices/Choice.hpp"
#include "NativeFixtures.hpp"
#include <Optimization/SrtWalker/SrtEvaluator.hpp>
#include <array>

namespace {
using namespace ShaderRecompiler;

struct Fixture {
    IrProgram program;
    IrValue* handle = nullptr;
    IrValue* firstRead = nullptr;
    IrBlock* yes = nullptr;

    explicit Fixture(IrOpcode opcode, unsigned words, bool uniform = true) {
        auto& entry = program.CreateBlock();
        yes = &program.CreateBlock();
        auto& no = program.CreateBlock();
        auto& join = program.CreateBlock();
        program.SetEntryBlock(entry);
        program.BlockOrder() = {&entry, yes, &no, &join};
        program.Resources().userDataCount = 2;
        entry.AddBranch(yes);
        entry.AddBranch(&no);
        yes->AddBranch(&join);
        no.AddBranch(&join);
        program.Metadata().blockInfo.resize(4);
        for (unsigned index = 0; index < 4; ++index) program.Metadata().blockInfo[index].id = index;
        auto& metadata = program.Metadata().blockInfo;
        metadata[0].terminator.kind = TerminatorKind::ConditionalBranch;
        metadata[0].terminator.trueBlock = 1;
        metadata[0].terminator.falseBlock = 2;
        metadata[1].terminator.kind = metadata[2].terminator.kind = TerminatorKind::Branch;
        metadata[1].terminator.trueBlock = metadata[2].terminator.trueBlock = 3;
        IrBuilder builder(program);
        builder.SetInsertionPoint(entry);
        auto& value = uniform ? builder.GetUserData(static_cast<ScalarReg>(0)) : builder.Emit(IrOpcode::LaneId, IrType::U32, {});
        metadata[0].condition = &builder.IEqual(value, builder.Constant(0));
        auto& low = builder.GetUserData(static_cast<ScalarReg>(1));
        auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &builder.Constant(0)});
        std::array<std::array<IrValue*, 8>, 2> reads{};
        for (unsigned arm = 0; arm < 2; ++arm) {
            builder.SetInsertionPoint(arm == 0 ? *yes : no);
            for (unsigned word = 0; word < words; ++word) {
                MemoryInfo memory;
                memory.kind = ResourceKind::ScalarAddress;
                memory.offset = (arm == 0 ? 0xE0 : 0xC0) + word * 4;
                MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
                program.Resources().memoryInfo.push_back(memory);
                reads[arm][word] = &builder.Emit(IrOpcode::LoadAddressU32, IrType::U32, {&address, &builder.Constant(0), &builder.Constant(0), &builder.ConstantBool(true)}, flags);
            }
        }
        firstRead = reads[0][0];
        builder.SetInsertionPoint(join);
        std::vector<IrValue*> phis;
        for (unsigned word = 0; word < words; ++word) {
            auto& phi = builder.Emit(IrOpcode::Phi, IrType::U32, {});
            if (word % 2 == 0) {
                phi.AddPhiOperand(yes, reads[0][word]);
                phi.AddPhiOperand(&no, reads[1][word]);
            } else {
                phi.AddPhiOperand(&no, reads[1][word]);
                phi.AddPhiOperand(yes, reads[0][word]);
            }
            phis.push_back(&phi);
        }
        handle = &program.CreateValue(opcode, IrOpcodeType(opcode));
        for (auto* phi : phis) handle->AddArgument(phi);
        join.AppendInstruction(handle);
        ValidateProgram(program, true);
    }
};

struct Reads {
    std::uint64_t begin = 0;
    unsigned words = 0;
    unsigned count = 0;
};

bool Read(void* user, std::uint64_t address, std::uint32_t* value) {
    auto& reads = *static_cast<Reads*>(user);
    if (address < reads.begin || address - reads.begin >= reads.words * 4 || address % 4 != 0) return false;
    *value = static_cast<std::uint32_t>(0xA5000000 | address);
    ++reads.count;
    return true;
}
}

void CheckDescriptorChoices() {
    using namespace Fixtures;
    for (auto opcode : {IrOpcode::GetImageResource, IrOpcode::GetBufferResource, IrOpcode::GetSamplerResource}) {
        unsigned words = opcode == IrOpcode::GetImageResource ? 8 : 4;
        Fixture fixture(opcode, words);
        Require(AnyBox::Shader::PlanDescriptorChoices(fixture.program) == 1);
        ValidateProgram(fixture.program, true);
        for (auto selector : {0u, 1u, UINT32_MAX}) {
            std::array<std::uint32_t, 2> userData{selector, 0x1000};
            Reads reads{selector == 0 ? 0x10E0u : 0x10C0u, words, 0};
            SrtRuntime runtime;
            runtime.userData = userData;
            runtime.readMemory = Read;
            runtime.userContext = &reads;
            Detail::Evaluator evaluator(fixture.program.Resources(), runtime);
            for (unsigned word = 0; word < words; ++word) {
                std::uint32_t value = 0;
                Require(evaluator.Evaluate(Detail::IrNode(fixture.handle->Argument(word)), value));
                Require(value == (0xA5000000 | static_cast<std::uint32_t>(reads.begin + word * 4)));
            }
            Require(reads.count == words);
        }
    }
    Fixture varying(IrOpcode::GetImageResource, 8, false);
    Require(AnyBox::Shader::PlanDescriptorChoices(varying.program) == 0);
    Fixture coherent(IrOpcode::GetImageResource, 8);
    coherent.program.Resources().memoryInfo.front().coherent = true;
    Require(AnyBox::Shader::PlanDescriptorChoices(coherent.program) == 0);
    Fixture barrier(IrOpcode::GetImageResource, 8);
    IrBuilder builder(barrier.program);
    builder.SetInsertionPoint(*barrier.yes);
    (void)builder.Emit(IrOpcode::Barrier, IrType::Void, {});
    Require(AnyBox::Shader::PlanDescriptorChoices(barrier.program) == 0);
    Fixture shared(IrOpcode::GetImageResource, 8);
    IrBuilder sharedBuilder(shared.program);
    sharedBuilder.SetInsertionPoint(*shared.yes);
    (void)sharedBuilder.Emit(IrOpcode::ReferenceU32, IrType::Void, {shared.firstRead});
    Require(AnyBox::Shader::PlanDescriptorChoices(shared.program) == 0);
    Fixture hole(IrOpcode::GetImageResource, 8);
    hole.program.Resources().memoryInfo.at(1).offset += 4;
    Require(AnyBox::Shader::PlanDescriptorChoices(hole.program) == 0);
}
