#include "Fixture.hpp"

void StructuredTests::CheckExits(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    Fixture fixture;
    auto& builder = fixture.builder;
    auto& header = fixture.Block();
    auto& body = fixture.Block();
    auto& update = fixture.Block();
    auto& left = fixture.Block();
    auto& right = fixture.Block();
    auto& exit = fixture.Block();
    auto& stop = builder.BitwiseAnd(*fixture.index, builder.Constant(3));
    fixture.Branch(*fixture.entry, header);
    fixture.Enter(header);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& sum = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    fixture.Conditional(header, builder.ULessThan(counter, builder.Constant(4)), body, exit);
    fixture.Enter(body);
    fixture.Conditional(body, builder.IEqual(counter, stop), exit, update);
    fixture.Enter(update);
    auto& next = builder.IAdd(counter, builder.Constant(1));
    auto& parity = builder.BitwiseAnd(builder.IAdd(counter, *fixture.index), builder.Constant(1));
    fixture.Conditional(update, builder.IEqual(parity, builder.Constant(0)), left, right);
    fixture.Enter(left);
    auto& larger = builder.IAdd(sum, builder.Constant(10));
    fixture.Branch(left, header);
    fixture.Enter(right);
    auto& smaller = builder.IAdd(sum, builder.Constant(1));
    fixture.Branch(right, header);
    counter.AddPhiOperand(fixture.entry, &builder.Constant(0));
    counter.AddPhiOperand(&left, &next);
    counter.AddPhiOperand(&right, &next);
    sum.AddPhiOperand(fixture.entry, fixture.index);
    sum.AddPhiOperand(&left, &larger);
    sum.AddPhiOperand(&right, &smaller);
    fixture.Enter(exit);
    fixture.Return(exit, builder.IAdd(builder.IMul(counter, builder.Constant(10000)), sum));
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) {
        auto count = index & 3;
        auto total = index;
        for (unsigned step = 0; step < count; ++step) total += ((step + index) & 1) == 0 ? 10 : 1;
        expected[index] = count * 10000 + total;
    }
    fixture.Run(context, expected);
}
