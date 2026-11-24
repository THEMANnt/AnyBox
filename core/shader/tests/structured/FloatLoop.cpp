#include "Fixture.hpp"

void StructuredTests::CheckFloatLoop(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    Fixture fixture;
    auto& builder = fixture.builder;
    auto& header = fixture.Block();
    auto& body = fixture.Block();
    auto& exit = fixture.Block();
    auto& bound = builder.BitwiseAnd(*fixture.index, builder.Constant(3));
    auto& a = builder.BitCastF32(builder.Constant(0x7F812345));
    auto& b = builder.BitCastF32(builder.Constant(0x80000000U));
    fixture.Branch(*fixture.entry, header);
    fixture.Enter(header);
    auto& counter = builder.Emit(IrOpcode::Phi, IrType::U32, {});
    auto& first = builder.Emit(IrOpcode::Phi, IrType::F32, {});
    auto& second = builder.Emit(IrOpcode::Phi, IrType::F32, {});
    fixture.Conditional(header, builder.ULessThan(counter, bound), body, exit);
    fixture.Enter(body);
    auto& next = builder.IAdd(counter, builder.Constant(1));
    fixture.Branch(body, header);
    counter.AddPhiOperand(fixture.entry, &builder.Constant(0));
    counter.AddPhiOperand(&body, &next);
    first.AddPhiOperand(fixture.entry, &a);
    first.AddPhiOperand(&body, &second);
    second.AddPhiOperand(fixture.entry, &b);
    second.AddPhiOperand(&body, &first);
    fixture.Enter(exit);
    auto& countBits = builder.ShiftLeftLogical(counter, builder.Constant(16));
    fixture.Return(exit, builder.BitwiseXor(builder.BitCastU32(first), countBits));
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) {
        auto count = index & 3;
        auto bits = (count & 1) == 0 ? 0x7F812345U : 0x80000000U;
        expected[index] = bits ^ (count << 16);
    }
    fixture.Run(context, expected);
}
