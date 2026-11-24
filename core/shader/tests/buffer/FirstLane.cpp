#include "Target.hpp"
#include "Fixtures.hpp"
#include "../../src/native/FirstLane.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <Recompiler.hpp>
#include <array>

namespace {
enum class MaskKind { Boolean, Bits, Selection, BitSelection, Ballot, Compound, CompoundBallot };

ShaderRecompiler::IrProgram Make(bool optimize, MaskKind kind) {
    using namespace ShaderRecompiler;
    bool bits = kind == MaskKind::Bits || kind == MaskKind::BitSelection;
    bool selectedMask = kind == MaskKind::Selection || kind == MaskKind::BitSelection;
    bool ballotMask = kind == MaskKind::Ballot || kind == MaskKind::CompoundBallot;
    bool compoundMask = kind == MaskKind::Compound || kind == MaskKind::CompoundBallot;
    IrProgram program;
    program.SetWaveSize(32);
    auto& entry = program.CreateBlock();
    auto& header = program.CreateBlock();
    auto& body = program.CreateBlock();
    auto& exit = program.CreateBlock();
    program.SetEntryBlock(entry);
    program.BlockOrder() = {&entry, &header, &body, &exit};
    entry.AddBranch(&header);
    header.AddBranch(&body);
    header.AddBranch(&exit);
    body.AddBranch(&header);
    auto& metadata = program.Metadata().blockInfo;
    metadata.resize(4);
    for (unsigned index = 0; index < 4; ++index) metadata[index].id = index;
    metadata[0].terminator.kind = metadata[2].terminator.kind = TerminatorKind::Branch;
    metadata[0].terminator.trueBlock = metadata[2].terminator.trueBlock = 1;
    metadata[1].terminator.kind = TerminatorKind::ConditionalBranch;
    metadata[1].terminator.loopHeader = true;
    metadata[1].terminator.trueBlock = 2;
    metadata[1].terminator.falseBlock = metadata[1].terminator.mergeBlock = 3;
    metadata[1].terminator.continueBlock = 2;
    IrBuilder builder(program);
    builder.SetInsertionPoint(entry);
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto bit = [&](IrValue& word) -> IrValue& {
        auto& selector = builder.BitwiseAnd(builder.Emit(IrOpcode::LaneId, IrType::U32, {}), builder.Constant(31));
        return builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(word, selector), builder.Constant(1)), builder.Constant(0));
    };
    auto& lower = builder.LogicalNot(builder.ULessThan(lane, builder.Constant(16)));
    auto& upper = builder.ULessThan(lane, builder.Constant(28));
    auto& eligible = compoundMask ? builder.LogicalAnd(lower, upper) : bits ? bit(builder.Constant(0xFFFF0000U)) : lower;
    auto& entryBallot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&eligible});
    auto& ballotBits = builder.BitwiseAnd(builder.CompositeExtract(entryBallot, 0), builder.Constant(0xFF0F0000U));
    auto& base = ballotMask ? bit(ballotBits) : bits ? bit(builder.Constant(0xFF0F0000U)) : eligible;
    auto& selector = builder.INotEqual(builder.BitwiseAnd(lane, builder.Constant(1)), builder.Constant(0));
    auto& narrower = builder.LogicalAnd(base, builder.UGreaterThan(lane, builder.Constant(19)));
    auto& remaining = builder.LogicalAnd(upper, builder.INotEqual(lane, builder.Constant(19)));
    auto* initial = &base;
    if (compoundMask && !ballotMask) initial = &builder.LogicalAnd(lower, remaining);
    else if (selectedMask) initial = &builder.Select(selector, narrower, base);
    auto& payload = builder.ShiftLeftLogical(builder.IAdd(lane, builder.Constant(1)), builder.Constant(5));
    auto& low = builder.ShiftLeftLogical(lane, builder.Constant(2));
    builder.SetInsertionPoint(header);
    auto& mask = builder.Emit(IrOpcode::Phi, IrType::Bool, {});
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&mask});
    metadata[1].condition = &builder.INotEqual(builder.CompositeExtract(ballot, 0), builder.Constant(0));
    (void)builder.Emit(IrOpcode::Reference, IrType::Void, {metadata[1].condition});
    builder.SetInsertionPoint(body);
    auto& selected = builder.Select(eligible, payload, builder.Constant(3));
    auto& first = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&selected, &mask});
    auto& chosen = builder.LogicalAnd(mask, builder.IEqual(payload, first));
    auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &builder.Constant(2)});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&address, &low, &builder.Constant(2), &first, &chosen}, MemoryFlags{0, 0});
    auto& next = builder.LogicalAnd(mask, builder.LogicalNot(chosen));
    mask.AddPhiOperand(&entry, initial);
    mask.AddPhiOperand(&body, &next);
    ValidateProgram(program, true);
    if (optimize) Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(program) == 1);
    return program;
}
}

void BufferTests::CheckFirstLaneGpu(AnyBox::Vulkan::Context& context) {
    for (auto kind : {MaskKind::Boolean, MaskKind::Bits, MaskKind::Selection, MaskKind::BitSelection, MaskKind::Ballot, MaskKind::Compound, MaskKind::CompoundBallot}) {
        bool bits = kind == MaskKind::Bits || kind == MaskKind::BitSelection;
        bool selectedMask = kind == MaskKind::Selection || kind == MaskKind::BitSelection;
        bool ballotMask = kind == MaskKind::Ballot || kind == MaskKind::CompoundBallot;
        bool compoundMask = kind == MaskKind::Compound || kind == MaskKind::CompoundBallot;
        for (bool optimize : {false, true}) {
            auto program = Make(optimize, kind);
            ShaderRecompiler::ShaderComputeInputInfo input;
            input.threadsNum[0] = 32;
            input.threadsNum[1] = input.threadsNum[2] = 1;
            input.waveSize = input.hostSubgroupSize = 32;
            auto shader = AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128});
            std::array<std::uint32_t, 32> expected;
            expected.fill(UINT32_MAX);
            for (unsigned lane = 0; lane < 32; ++lane) {
                bool active = (((bits || ballotMask) ? 0xFF0F0000U : 0xFFFF0000U) & (1U << lane)) != 0;
                if (selectedMask && (lane & 1) != 0 && lane <= 19) active = false;
                if (compoundMask) active = lane >= 16 && lane < 28 && (ballotMask ? (0xFF0F0000U & (1U << lane)) != 0 : lane != 19);
                if (active) expected[lane] = (lane + 1) << 5;
            }
            RunGpu(context, shader, expected, {1, 1, 1});
        }
    }
}
