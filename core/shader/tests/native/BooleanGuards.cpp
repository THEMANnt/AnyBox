#include "../../src/native/FirstLane.hpp"
#include "NativeFixtures.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>

namespace {
void Check(bool disjunction, bool complete) {
    using namespace ShaderRecompiler;
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    auto predicate = [&](unsigned index) -> IrValue& {
        return builder.INotEqual(builder.GetUserData(static_cast<ScalarReg>(index)), builder.Constant(0));
    };
    auto& first = predicate(0);
    auto& second = predicate(1);
    auto& other = predicate(2);
    auto& condition = disjunction ? builder.LogicalOr(first, second) : builder.LogicalAnd(first, second);
    auto& nested = builder.LogicalAnd(second, other);
    auto* mask = &other;
    if (disjunction && complete) mask = &builder.LogicalAnd(second, other);
    if (!disjunction) mask = complete ? &builder.LogicalAnd(first, nested) : &builder.LogicalAnd(first, other);
    auto& payload = builder.Constant(32);
    auto& value = builder.Select(condition, payload, builder.Constant(3));
    auto& read = builder.Emit(IrOpcode::ReadFirstLane, IrType::U32, {&value, mask});
    ValidateProgram(program, true);

    Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(program) == (complete ? 1 : 0));
    Fixtures::Require(read.Argument(0) == (complete ? &payload : &value) && read.Argument(1) == mask);
    ValidateProgram(program, true);
}
}

void CheckBooleanFirstLaneGuards() {
    for (bool disjunction : {false, true}) {
        for (bool complete : {false, true}) Check(disjunction, complete);
    }
}
