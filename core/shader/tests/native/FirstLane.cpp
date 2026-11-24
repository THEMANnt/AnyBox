#include "../../src/native/FirstLane.hpp"
#include "NativeFixtures.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

namespace {
using namespace ShaderRecompiler;

struct Fixture {
    IrProgram program;
    IrValue* mask = nullptr;
    IrValue* read = nullptr;
    IrValue* payload = nullptr;

    Fixture(bool narrow, bool duplicate, bool changing, bool validEntry, bool bits = false, bool widenedBits = false, unsigned selectorMask = 31) {
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
        auto bit = [&](IrValue& word, unsigned selector) -> IrValue& {
            auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
            auto& index = builder.BitwiseAnd(lane, builder.Constant(selector));
            return builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(word, index), builder.Constant(1)), builder.Constant(0));
        };
        auto& raw = builder.GetUserData(static_cast<ScalarReg>(0));
        auto& condition = bits ? bit(raw, 31) : builder.INotEqual(raw, builder.Constant(0));
        auto& additional = builder.GetUserData(static_cast<ScalarReg>(2));
        auto& initialBits = widenedBits ? builder.BitwiseOr(raw, additional) : builder.BitwiseAnd(raw, additional);
        auto& anchored = bits ? bit(initialBits, selectorMask) : condition;
        auto& other = builder.INotEqual(builder.GetUserData(static_cast<ScalarReg>(1)), builder.Constant(0));
        builder.SetInsertionPoint(header);
        mask = &builder.Emit(IrOpcode::Phi, IrType::Bool, {});
        auto* copied = duplicate ? &builder.Emit(IrOpcode::Phi, IrType::Bool, {}) : mask;
        metadata[1].condition = mask;
        builder.SetInsertionPoint(body);
        auto& chosen = changing ? builder.LogicalNot(*mask) : condition;
        payload = &builder.Constant(32);
        auto& value = builder.Select(chosen, *payload, builder.Constant(3));
        read = &builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&value, mask});
        auto& update = narrow ? builder.LogicalAnd(*copied, other) : builder.LogicalOr(*copied, other);
        auto* initial = validEntry ? &anchored : &other;
        mask->AddPhiOperand(&entry, initial);
        mask->AddPhiOperand(&body, &update);
        if (duplicate) {
            copied->AddPhiOperand(&entry, initial);
            copied->AddPhiOperand(&body, &update);
        }
        ValidateProgram(program, true);
    }
};

void CheckSelectedMask(bool guarded) {
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    auto& condition = builder.INotEqual(builder.GetUserData(static_cast<ScalarReg>(0)), builder.Constant(0));
    auto& selector = builder.INotEqual(builder.GetUserData(static_cast<ScalarReg>(1)), builder.Constant(0));
    auto& other = builder.INotEqual(builder.GetUserData(static_cast<ScalarReg>(2)), builder.Constant(0));
    auto& narrow = builder.LogicalAnd(condition, other);
    auto& mask = builder.Select(selector, narrow, guarded ? condition : other);
    auto& payload = builder.Constant(32);
    auto& value = builder.Select(condition, payload, builder.Constant(3));
    auto& read = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&value, &mask});
    ValidateProgram(program, true);

    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(program) == (guarded ? 1 : 0));
    Fixtures::Require(read.Argument(0) == (guarded ? &payload : &value) && read.Argument(1) == &mask);
    ValidateProgram(program, true);
}
}

void CheckFirstLaneGuards() {
    CheckSelectedMask(true);
    CheckSelectedMask(false);
    Fixture unmarked(true, true, false, true);
    unmarked.program.Metadata().blockInfo[1].terminator.loopHeader = false;
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(unmarked.program) == 1);
    for (bool duplicate : {false, true}) {
        Fixture fixture(true, duplicate, false, true);
        Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(fixture.program) == 1);
        Fixtures::Require(fixture.read->Argument(0) == fixture.payload && fixture.read->Argument(1) == fixture.mask);
        ValidateProgram(fixture.program, true);
    }
    Fixture widening(false, false, false, true);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(widening.program) == 0);
    Fixture changing(true, false, true, true);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(changing.program) == 0);
    Fixture unguarded(true, false, false, false);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(unguarded.program) == 0);
    Fixture bits(true, true, false, true, true);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(bits.program) == 1);
    Fixture widenedBits(true, true, false, true, true, true);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(widenedBits.program) == 0);
    Fixture selector(true, true, false, true, true, false, 15);
    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(selector.program) == 0);
}
