#include "../../src/native/FirstLane.hpp"
#include "NativeFixtures.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

namespace {
void Check(std::uint32_t waveSize, std::uint32_t selectorMask, unsigned component, bool widen, bool expected, bool compound = false) {
    using namespace ShaderRecompiler;
    IrProgram program;
    program.SetWaveSize(waveSize);
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& lower = builder.UGreaterThan(lane, builder.Constant(8));
    auto& condition = compound ? builder.LogicalAnd(lower, builder.ULessThan(lane, builder.Constant(28))) : lower;
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {&condition});
    auto& word = builder.CompositeExtract(ballot, component);
    auto& extra = builder.Constant(0xFF0F0000U);
    auto& narrowed = widen ? builder.BitwiseOr(word, extra) : builder.BitwiseAnd(word, extra);
    auto& selector = builder.BitwiseAnd(lane, builder.Constant(selectorMask));
    auto& mask = builder.INotEqual(builder.BitwiseAnd(builder.ShiftRightLogical(narrowed, selector), builder.Constant(1)), builder.Constant(0));
    auto& payload = builder.Constant(32);
    auto& value = builder.Select(condition, payload, builder.Constant(3));
    auto& read = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&value, &mask});
    ValidateProgram(program, true);

    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(program) == (expected ? 1 : 0));
    Fixtures::Require(read.Argument(0) == (expected ? &payload : &value) && read.Argument(1) == &mask);
    ValidateProgram(program, true);
}
}

void CheckBallotFirstLaneGuards() {
    Check(32, 31, 0, false, true);
    Check(32, 31, 0, false, true, true);
    Check(32, UINT32_MAX, 0, false, true);
    Check(64, 31, 0, false, false);
    Check(32, 15, 0, false, false);
    Check(32, 31, 1, false, false);
    Check(32, 31, 0, true, false);
}
