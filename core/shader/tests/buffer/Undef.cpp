#include "FloatFixture.hpp"

void BufferTests::CheckUndefGpu(AnyBox::Vulkan::Context& context) {
    FloatFixture fixture;
    for (unsigned bits : {1u, 8u, 16u, 32u, 64u}) {
        auto* type = fixture.IntegerType(bits);
        auto* value = LLVMBC::UndefValue::get(type);
        if (bits == 1) {
            fixture.Expect(*fixture.Make<LLVMBC::SelectInst>(fixture.Integer(32, 1), fixture.Integer(32, 0), value), 0);
            continue;
        }
        auto* converted = bits == 32 ? static_cast<LLVMBC::Value*>(value) : fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), value, bits < 32 ? LLVMBC::Instruction::ZExt : LLVMBC::Instruction::Trunc);
        fixture.Expect(*converted, 0);
        auto* sum = fixture.Make<LLVMBC::BinaryOperator>(value, fixture.Integer(bits, 7), LLVMBC::Instruction::Add);
        auto* result = bits == 32 ? static_cast<LLVMBC::Value*>(sum) : fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), sum, bits < 32 ? LLVMBC::Instruction::ZExt : LLVMBC::Instruction::Trunc);
        fixture.Expect(*result, 7);
    }
    auto* floating = LLVMBC::UndefValue::get(fixture.FloatType());
    fixture.Expect(*floating, 0);
    fixture.Expect(*fixture.Make<LLVMBC::BinaryOperator>(floating, fixture.Argument(0), LLVMBC::Instruction::FAdd), 0x3F800000);
    fixture.Reject(*LLVMBC::UndefValue::get(fixture.IntegerType(24)));
    fixture.Reject(*fixture.Make<LLVMBC::Argument>(fixture.IntegerType(32), 99, false));
    fixture.Reject(*fixture.Make<LLVMBC::Argument>(fixture.FloatType(), 99, false));
    std::array<std::uint32_t, 16> input{};
    input[0] = 0x3F800000;
    fixture.Run(context, input);
}
