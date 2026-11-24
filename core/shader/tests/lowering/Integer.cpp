#include "Batch.hpp"
#include <anybox/ExceptionTest.hpp>

namespace {
void CheckInteger(unsigned bits) {
    using Op = LLVMBC::Instruction;
    LoweringTests::Batch batch;
    auto mask = static_cast<std::uint32_t>((std::uint64_t{1} << bits) - 1);
    auto sign = std::uint32_t{1} << (bits - 1);
    std::array<std::uint32_t, 8> inputs{0, 1, 2 & mask, 3 & mask, sign - 1, sign, mask - 1, mask};
    for (auto left : inputs) {
        for (auto right : inputs) {
            auto* a = batch.Constant(bits, left);
            auto* b = batch.Constant(bits, right);
            auto test = [&](Op::BinaryOps operation, std::uint32_t expected) {
                auto* expression = batch.Context().construct<LLVMBC::BinaryOperator>(a, b, operation);
                batch.Expect(*expression, expected & mask);
            };
            test(Op::Add, left + right);
            test(Op::Sub, left - right);
            test(Op::Mul, left * right);
            test(Op::And, left & right);
            test(Op::Or, left | right);
            test(Op::Xor, left ^ right);
            if (right < bits) {
                test(Op::Shl, left << right);
                test(Op::LShr, left >> right);
                test(Op::AShr, static_cast<std::uint32_t>(LoweringTests::Signed(left, bits) >> right));
            }
            if (right != 0) {
                test(Op::UDiv, left / right);
                test(Op::URem, left % right);
                auto signedLeft = LoweringTests::Signed(left, bits);
                auto signedRight = LoweringTests::Signed(right, bits);
                if (signedLeft != -static_cast<std::int64_t>(sign) || signedRight != -1) {
                    test(Op::SDiv, static_cast<std::uint32_t>(signedLeft / signedRight));
                    test(Op::SRem, static_cast<std::uint32_t>(signedLeft % signedRight));
                }
            }
        }
    }
    batch.Verify();
}
}

int main() {
    return AnyBox::Tests::Run([] {
        for (unsigned bits : {1u, 8u, 16u, 32u}) CheckInteger(bits);
        LoweringTests::CheckComparisonsAndCasts();
        LoweringTests::CheckWideIntegers();
        LoweringTests::CheckIntrinsics();
    });
}
