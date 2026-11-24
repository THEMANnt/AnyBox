#include "Fixture.hpp"

void StructuredTests::CheckTyped(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    Fixture fixture;
    auto& builder = fixture.builder;
    auto& left = fixture.Block();
    auto& right = fixture.Block();
    auto& merge = fixture.Block();
    auto& even = builder.IEqual(builder.BitwiseAnd(*fixture.index, builder.Constant(1)), builder.Constant(0));
    fixture.Conditional(*fixture.entry, even, left, right);
    fixture.Enter(left);
    auto& lowLeft = builder.IAdd(*fixture.index, builder.Constant(7));
    auto& wideLeft = builder.ConstructU64(lowLeft, builder.Constant(0x12345678));
    auto& floatLeft = builder.BitCastF32(builder.Constant(0x7F812345));
    fixture.Branch(left, merge);
    fixture.Enter(right);
    auto& lowRight = builder.IMul(*fixture.index, builder.Constant(3));
    auto& wideRight = builder.ConstructU64(lowRight, builder.Constant(0xF1234567U));
    auto& floatRight = builder.BitCastF32(builder.Constant(0x80000000U));
    fixture.Branch(right, merge);
    fixture.Enter(merge);
    auto& wide = builder.Emit(IrOpcode::Phi, IrType::U64, {});
    wide.AddPhiOperand(&left, &wideLeft);
    wide.AddPhiOperand(&right, &wideRight);
    auto& floating = builder.Emit(IrOpcode::Phi, IrType::F32, {});
    floating.AddPhiOperand(&left, &floatLeft);
    floating.AddPhiOperand(&right, &floatRight);
    auto& boolean = builder.Emit(IrOpcode::Phi, IrType::Bool, {});
    boolean.AddPhiOperand(&left, &builder.ConstantBool(true));
    boolean.AddPhiOperand(&right, &builder.ConstantBool(false));
    auto& limbs = builder.BitwiseXor(builder.CompositeExtract(wide, 0), builder.CompositeExtract(wide, 1));
    auto& tagged = builder.BitwiseXor(limbs, builder.Select(boolean, builder.Constant(0xAAAA), builder.Constant(0x5555)));
    fixture.Return(merge, builder.BitwiseXor(tagged, builder.BitCastU32(floating)));
    std::array<std::uint32_t, 32> expected;
    for (unsigned index = 0; index < expected.size(); ++index) {
        expected[index] = (index & 1) == 0 ? 0x7F812345U ^ (index + 7) ^ 0x12345678U ^ 0xAAAA : 0x80000000U ^ (index * 3) ^ 0xF1234567U ^ 0x5555;
    }
    fixture.Run(context, expected);
}
