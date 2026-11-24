#include "Fixture.hpp"

namespace {
void CheckDiamond(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    StructuredTests::Fixture fixture;
    auto& builder = fixture.builder;
    auto& merge = fixture.Block();
    auto& left = fixture.Block();
    auto& right = fixture.Block();
    auto& even = builder.IEqual(builder.BitwiseAnd(*fixture.index, builder.Constant(1)), builder.Constant(0));
    fixture.Conditional(*fixture.entry, even, left, right);
    fixture.Enter(left);
    auto& first = builder.IAdd(*fixture.index, builder.Constant(7));
    fixture.Branch(left, merge);
    fixture.Enter(right);
    auto& second = builder.IMul(*fixture.index, builder.Constant(3));
    fixture.Branch(right, merge);
    fixture.Enter(merge);
    auto& phi = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    phi.AddPhiOperand(&left, &first);
    phi.AddPhiOperand(&right, &second);
    fixture.Return(merge, phi);
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) expected[index] = (index & 1) == 0 ? index + 7 : index * 3;
    fixture.Run(context, expected);
}

void CheckReturns(AnyBox::Vulkan::Context& context) {
    StructuredTests::Fixture fixture;
    auto& builder = fixture.builder;
    auto& left = fixture.Block();
    auto& right = fixture.Block();
    auto& condition = builder.ULessThan(*fixture.index, builder.Constant(16));
    fixture.Conditional(*fixture.entry, condition, left, right);
    fixture.Enter(left);
    fixture.Return(left, builder.IAdd(*fixture.index, builder.Constant(10)));
    fixture.Enter(right);
    fixture.Return(right, builder.IMul(*fixture.index, builder.Constant(2)));
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) expected[index] = index < 16 ? index + 10 : index * 2;
    fixture.Run(context, expected);
}
}

void StructuredTests::CheckBranches(AnyBox::Vulkan::Context& context) {
    CheckDiamond(context);
    CheckReturns(context);
}
