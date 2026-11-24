#include "FloatFixture.hpp"
#include "FloatCallKernel.hpp"
#include <Recompiler.hpp>
#include <bit>
#include <cmath>

namespace {
std::uint32_t Expected(std::uint32_t left, std::uint32_t right, bool maximum) {
    auto flush = [](std::uint32_t value) { return (value & 0x7F800000U) == 0 ? value & 0x80000000U : value; };
    left = flush(left);
    right = flush(right);
    auto a = std::bit_cast<float>(left);
    auto b = std::bit_cast<float>(right);
    if (std::isnan(a) && std::isnan(b)) return 0x7FC00000U;
    if (std::isnan(a)) return right;
    if (std::isnan(b)) return left;
    if (((left | right) & 0x7FFFFFFFU) == 0) return maximum ? left & right : left | right;
    return std::bit_cast<std::uint32_t>(maximum ? std::fmax(a, b) : std::fmin(a, b));
}

void Expect(BufferTests::FloatFixture& fixture, LLVMBC::Value* left, LLVMBC::Value* right, unsigned opcode, std::uint32_t reference) {
    auto* result = fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(32, opcode), left, right});
    auto* nan = fixture.Make<LLVMBC::FCmpInst>(LLVMBC::Instruction::FCMP_UNO, result, result);
    auto* canonical = fixture.Make<LLVMBC::SelectInst>(fixture.FloatConstant(0x7FC00000U), result, nan);
    fixture.Expect(*canonical, reference);
}
}

void BufferTests::CheckFloatMinMax(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0x3F800000, 0xC0000000, 0x3F000000, 0x4B800000, 0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 1, 0x80000001, 0x007FFFFF, 0x807FFFFF, 0x00800000, 0x7F7FFFFF};
    for (unsigned opcode : {35u, 36u}) {
        auto program = MakeFloatCallKernel(opcode, 2);
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        std::vector<std::uint32_t> expected;
        for (unsigned left = 0; left < input.size(); ++left) {
            for (unsigned right = 0; right < input.size(); ++right) expected.push_back(Expected(input[left], input[right], opcode == 35));
        }
        RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(input)));
    }
    FloatFixture fixture;
    for (unsigned opcode : {35u, 36u}) {
        for (std::uint32_t left : {0U, 0x80000000U}) {
            for (unsigned right : {4u, 5u, 8u, 9u, 10u, 11u}) Expect(fixture, fixture.FloatConstant(left), fixture.Argument(right), opcode, Expected(left, input[right], opcode == 35));
        }
    }
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(32, 35), fixture.Argument(0)}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(32, 35), fixture.Argument(0), fixture.Argument(1), fixture.Argument(2)}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(32, 34), fixture.Argument(0), fixture.Argument(1)}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.IntegerType(32), {fixture.Integer(32, 35), fixture.Argument(0), fixture.Argument(1)}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(16, 35), fixture.Argument(0), fixture.Argument(1)}));
    fixture.Reject(*fixture.Call("dx.op.binary.f32", fixture.FloatType(), {fixture.Integer(32, 35), fixture.Integer(32, 0), fixture.Argument(1)}));
    fixture.Run(context, input);
}
