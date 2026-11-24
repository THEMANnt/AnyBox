#include "WaveKernel.hpp"
#include "Fixtures.hpp"

void BufferTests::CheckWaveReductionInvalid() {
    FloatFixture fixture;
    auto* floating = fixture.FloatType();
    auto* value = fixture.Argument(0);
    auto* opcode = fixture.Integer(32, 119);
    auto* minimum = fixture.Integer(8, 2);
    auto* signedness = fixture.Integer(8, 0);
    auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        fixture.Reject(*fixture.Call("dx.op.waveActiveOp.f32", result, std::move(arguments)));
    };
    reject(floating, {});
    reject(floating, {opcode, value, minimum});
    reject(floating, {opcode, value, minimum, signedness, signedness});
    reject(fixture.IntegerType(32), {opcode, value, minimum, signedness});
    reject(floating, {fixture.Integer(16, 119), value, minimum, signedness});
    reject(floating, {fixture.Integer(32, 118), value, minimum, signedness});
    reject(floating, {opcode, fixture.Integer(32, 0), minimum, signedness});
    reject(floating, {opcode, value, fixture.Integer(32, 2), signedness});
    reject(floating, {opcode, value, minimum, fixture.Integer(32, 0)});
    reject(floating, {opcode, value, minimum, fixture.Integer(8, 1)});
    for (unsigned operation : {0u, 1u, 4u, 255u}) reject(floating, {opcode, value, fixture.Integer(8, operation), signedness});
    auto* dynamicWord = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), value, LLVMBC::Instruction::BitCast);
    auto* dynamicByte = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(8), dynamicWord, LLVMBC::Instruction::Trunc);
    reject(floating, {dynamicWord, value, minimum, signedness});
    reject(floating, {opcode, value, dynamicByte, signedness});
    reject(floating, {opcode, value, minimum, dynamicByte});
}
