#include "FloatFixture.hpp"

namespace BufferTests {
void CheckGlobalAtomicInvalid() {
    FloatFixture fixture;
    auto* floating = fixture.FloatType();
    auto* address = fixture.Integer(64, 0x200000000);
    auto* value = fixture.Argument(0);
    for (bool maximum : {false, true}) {
        auto* opcode = fixture.Integer(32, maximum ? 369 : 368);
        auto reject = [&](LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
            fixture.Reject(*fixture.Call(maximum ? "xbox.global_atomic_fmax" : "xbox.global_atomic_fmin", result, std::move(arguments)));
        };
        reject(floating, {});
        reject(floating, {opcode, address});
        reject(floating, {opcode, address, value, value});
        reject(fixture.IntegerType(32), {opcode, address, value});
        reject(floating, {fixture.Integer(16, maximum ? 369 : 368), address, value});
        reject(floating, {fixture.Integer(32, maximum ? 368 : 369), address, value});
        reject(floating, {opcode, fixture.Integer(32, 0), value});
        reject(floating, {opcode, address, fixture.Integer(32, 0)});
        auto* dynamic = fixture.Make<LLVMBC::CastInst>(fixture.IntegerType(32), value, LLVMBC::Instruction::BitCast);
        reject(floating, {dynamic, address, value});
    }
}
}
