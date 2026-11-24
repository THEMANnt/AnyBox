#include "Fixture.hpp"

void StructuredTests::CheckLoop(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    Fixture fixture;
    auto& builder = fixture.builder;
    auto& header = fixture.Block();
    auto& body = fixture.Block();
    auto& exit = fixture.Block();
    auto& bound = builder.BitwiseAnd(*fixture.index, builder.Constant(3));
    auto& other = builder.IAdd(*fixture.index, builder.Constant(100));
    fixture.Branch(*fixture.entry, header);
    fixture.Enter(header);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& first = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& second = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& condition = builder.ULessThan(counter, bound);
    fixture.Conditional(header, condition, body, exit);
    fixture.Enter(body);
    auto& next = builder.IAdd(counter, builder.Constant(1));
    fixture.Branch(body, header);
    counter.AddPhiOperand(fixture.entry, &builder.Constant(0));
    counter.AddPhiOperand(&body, &next);
    first.AddPhiOperand(fixture.entry, fixture.index);
    first.AddPhiOperand(&body, &second);
    second.AddPhiOperand(fixture.entry, &other);
    second.AddPhiOperand(&body, &first);
    fixture.Enter(exit);
    auto& pair = builder.IAdd(builder.IMul(first, builder.Constant(1000)), second);
    auto& result = builder.IAdd(builder.IMul(counter, builder.Constant(1000000)), pair);
    fixture.Return(exit, result);
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) {
        auto count = index & 3;
        auto a = (count & 1) != 0 ? index + 100 : index;
        auto b = (count & 1) != 0 ? index : index + 100;
        expected[index] = count * 1000000 + a * 1000 + b;
    }
    fixture.Run(context, expected);
}
