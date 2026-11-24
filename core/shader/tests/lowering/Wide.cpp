#include "Batch.hpp"
#include "Fixtures.hpp"
#include <bit>

void LoweringTests::CheckWideIntegers() {
    using Op = LLVMBC::Instruction;
    Batch batch;
    std::array<std::uint64_t, 10> inputs{0, 1, 31, 32, 63, 0xFFFFFFFF, 0x100000000, 0x7FFFFFFFFFFFFFFF, 0x8000000000000000, UINT64_MAX};
    for (auto left : inputs) {
        auto* a = batch.Constant(64, left);
        for (auto right : inputs) {
            auto* b = batch.Constant(64, right);
            auto test = [&](Op::BinaryOps operation, std::uint64_t expected) {
                auto* expression = batch.Context().construct<LLVMBC::BinaryOperator>(a, b, operation);
                batch.Expect(*expression, expected);
            };
            test(Op::Add, left + right);
            test(Op::Sub, left - right);
            test(Op::Mul, left * right);
            test(Op::And, left & right);
            test(Op::Or, left | right);
            test(Op::Xor, left ^ right);
            if (right < 64) {
                test(Op::Shl, left << right);
                test(Op::LShr, left >> right);
                test(Op::AShr, static_cast<std::uint64_t>(std::bit_cast<std::int64_t>(left) >> right));
            }
            auto signedLeft = std::bit_cast<std::int64_t>(left);
            auto signedRight = std::bit_cast<std::int64_t>(right);
            std::array<bool, 10> expected{left == right, left != right, left > right, left >= right, left < right, left <= right, signedLeft > signedRight, signedLeft >= signedRight, signedLeft < signedRight, signedLeft <= signedRight};
            for (unsigned index = 0; index < expected.size(); ++index) {
                auto* comparison = batch.Context().construct<LLVMBC::ICmpInst>(static_cast<Op::Predicate>(Op::ICMP_EQ + index), a, b);
                batch.Expect(*comparison, expected[index]);
            }
        }
        for (unsigned target : {1u, 8u, 16u, 32u}) {
            auto* type = LLVMBC::Type::getIntTy(batch.Context(), target);
            auto* truncation = batch.Context().construct<LLVMBC::CastInst>(type, a, Op::Trunc);
            auto mask = (std::uint64_t{1} << target) - 1;
            batch.Expect(*truncation, left & mask);
            auto* source = batch.Constant(target, left & mask);
            auto* wide = LLVMBC::Type::getInt64Ty(batch.Context());
            auto* zeroExtension = batch.Context().construct<LLVMBC::CastInst>(wide, source, Op::ZExt);
            batch.Expect(*zeroExtension, left & mask);
            auto* signExtension = batch.Context().construct<LLVMBC::CastInst>(wide, source, Op::SExt);
            batch.Expect(*signExtension, static_cast<std::uint64_t>(Signed(static_cast<std::uint32_t>(left & mask), target)));
        }
        auto* selected = batch.Context().construct<LLVMBC::SelectInst>(a, batch.Constant(64, ~left), batch.Constant(1, 1));
        batch.Expect(*selected, left);
    }
    for (auto operation : {Op::UDiv, Op::SDiv, Op::URem, Op::SRem}) {
        auto* expression = batch.Context().construct<LLVMBC::BinaryOperator>(batch.Constant(64, 7), batch.Constant(64, 2), operation);
        Fixtures::Reject<std::runtime_error>([&] { batch.Lowering().Get(*expression); });
    }
    batch.Verify();
}
