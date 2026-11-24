#include "FloatFixture.hpp"
#include "FloatCallKernel.hpp"
#include "Fixtures.hpp"
#include <Recompiler.hpp>
#include <bit>
#include <cmath>
#include <unordered_set>

namespace {
std::uint32_t Expected(std::uint32_t left, std::uint32_t right, std::uint32_t addend) {
    volatile float product = std::bit_cast<float>(left) * std::bit_cast<float>(right);
    float result = product + std::bit_cast<float>(addend);
    return std::isnan(result) ? 0x7FC00000U : std::bit_cast<std::uint32_t>(result);
}

void CheckRounding(std::span<const std::uint32_t> code) {
    std::unordered_set<std::uint32_t> exact;
    std::vector<std::uint32_t> arithmetic;
    for (std::size_t offset = 5; offset < code.size();) {
        auto words = code[offset] >> 16;
        auto opcode = code[offset] & 65535;
        Fixtures::Require(words != 0 && words <= code.size() - offset);
        if (opcode == spv::OpDecorate && words >= 3 && code[offset + 2] == spv::DecorationNoContraction) exact.insert(code[offset + 1]);
        if (opcode == spv::OpFAdd || opcode == spv::OpFMul) {
            Fixtures::Require(words == 5);
            arithmetic.push_back(code[offset + 2]);
        }
        offset += words;
    }
    Fixtures::Require(arithmetic.size() == 2);
    for (auto id : arithmetic) Fixtures::Require(exact.contains(id));
}

void CheckInvalid(AnyBox::Vulkan::Context& context, std::span<const std::uint32_t> input) {
    BufferTests::FloatFixture fixture;
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call("dx.op.tertiary.f32", result, std::move(arguments)));
    };
    auto* floating = fixture.FloatType();
    auto* a = fixture.Argument(0);
    auto* b = fixture.Argument(1);
    auto* c = fixture.Argument(2);
    auto* opcode = fixture.Integer(32, 46);
    reject(floating, {});
    reject(floating, {opcode, a, b});
    reject(floating, {opcode, a, b, c, c});
    reject(fixture.IntegerType(32), {opcode, a, b, c});
    reject(floating, {fixture.Integer(16, 46), a, b, c});
    reject(floating, {fixture.Integer(32, 47), a, b, c});
    reject(floating, {fixture.Integer(32, 48), a, b, c});
    reject(floating, {a, a, b, c});
    auto* dynamic = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), a, LLVMBC::Instruction::BitCast);
    reject(floating, {dynamic, a, b, c});
    for (unsigned index = 1; index < 4; ++index) {
        LLVMBC::Vector<LLVMBC::Value*> arguments{opcode, a, b, c};
        arguments[index] = fixture.Integer(32, 0);
        reject(floating, std::move(arguments));
    }
    fixture.Expect(*fixture.Call("dx.op.tertiary.f32", floating, {opcode, a, b, c}), Expected(input[0], input[1], input[2]));
    fixture.Run(context, input);
}
}

void BufferTests::CheckFloatMad(AnyBox::Vulkan::Context& context) {
    std::array<std::uint32_t, 16> input{0x3F800001, 0x3F7FFFFE, 0xBF800000, 0, 0x80000000, 0x40000000, 0xC0000000, 0x3F000000, 0xBF000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0x7F812345, 0x477FE000, 0xC77FE000, 0x3EAAAAAB};
    auto fused = std::fma(std::bit_cast<float>(input[0]), std::bit_cast<float>(input[1]), std::bit_cast<float>(input[2]));
    Fixtures::Require(Expected(input[0], input[1], input[2]) == 0 && std::bit_cast<std::uint32_t>(fused) == 0xA8800000);

    auto program = MakeFloatCallKernel(46, 3);
    ShaderRecompiler::ShaderComputeInputInfo info;
    info.threadsNum[0] = 64;
    info.threadsNum[1] = info.threadsNum[2] = 1;
    info.waveSize = info.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    CheckRounding(shader.code);
    std::vector<std::uint32_t> expected;
    for (auto left : input) {
        for (auto right : input) {
            for (auto addend : input) expected.push_back(Expected(left, right, addend));
        }
    }
    RunGpu(context, shader, expected, {64, 1, 1}, std::as_bytes(std::span(input)));
    CheckInvalid(context, input);
}
