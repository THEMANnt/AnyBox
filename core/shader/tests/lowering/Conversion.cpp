#include "Batch.hpp"
#include "Fixtures.hpp"

void LoweringTests::CheckComparisonsAndCasts() {
    using Op = LLVMBC::Instruction;
    Batch batch;
    for (unsigned bits : {1u, 8u, 16u, 32u}) {
        auto mask = static_cast<std::uint32_t>((std::uint64_t{1} << bits) - 1);
        auto sign = std::uint32_t{1} << (bits - 1);
        std::array<std::uint32_t, 6> inputs{0, 1, sign - 1, sign, mask - 1, mask};
        for (auto left : inputs) {
            auto* a = batch.Constant(bits, left);
            for (auto right : inputs) {
                auto* b = batch.Constant(bits, right);
                auto signedLeft = Signed(left, bits);
                auto signedRight = Signed(right, bits);
                std::array<bool, 10> expected{left == right, left != right, left > right, left >= right, left < right, left <= right, signedLeft > signedRight, signedLeft >= signedRight, signedLeft < signedRight, signedLeft <= signedRight};
                for (unsigned index = 0; index < expected.size(); ++index) {
                    auto predicate = static_cast<Op::Predicate>(Op::ICMP_EQ + index);
                    auto* comparison = batch.Context().construct<LLVMBC::ICmpInst>(predicate, a, b);
                    batch.Expect(*comparison, expected[index]);
                }
            }
            for (unsigned target : {1u, 8u, 16u, 32u}) {
                auto* type = LLVMBC::Type::getIntTy(batch.Context(), target);
                if (target < bits) {
                    auto* truncation = batch.Context().construct<LLVMBC::CastInst>(type, a, Op::Trunc);
                    batch.Expect(*truncation, left & ((1u << target) - 1));
                } else if (target > bits) {
                    auto* zeroExtension = batch.Context().construct<LLVMBC::CastInst>(type, a, Op::ZExt);
                    batch.Expect(*zeroExtension, left);
                    auto* signExtension = batch.Context().construct<LLVMBC::CastInst>(type, a, Op::SExt);
                    auto targetMask = static_cast<std::uint32_t>((std::uint64_t{1} << target) - 1);
                    batch.Expect(*signExtension, static_cast<std::uint32_t>(Signed(left, bits)) & targetMask);
                }
            }
            auto* selected = batch.Context().construct<LLVMBC::SelectInst>(a, batch.Constant(bits, mask), batch.Constant(1, 1));
            batch.Expect(*selected, left);
        }
    }
    auto* wide = LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(batch.Context(), 128), 1);
    Fixtures::Reject<std::runtime_error>([&] { batch.Lowering().Get(*wide); });
    auto* invalid = batch.Context().construct<LLVMBC::CastInst>(LLVMBC::Type::getInt16Ty(batch.Context()), batch.Constant(16, 1), Op::ZExt);
    Fixtures::Reject<std::runtime_error>([&] { batch.Lowering().Get(*invalid); });
    batch.Verify();
}
