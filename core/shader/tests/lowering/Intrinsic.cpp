#include "Batch.hpp"
#include "Fixtures.hpp"
#include <module.hpp>
#include <function.hpp>

#undef llvm

void LoweringTests::CheckIntrinsics() {
    Batch batch;
    LLVMBC::Module module(batch.Context());
    module.add_value_name(0, "xbox.V_ALIGNBYTE_B32");
    auto* word = LLVMBC::Type::getInt32Ty(batch.Context());
    auto* type = batch.Context().construct<LLVMBC::FunctionType>(batch.Context(), word, LLVMBC::Vector<LLVMBC::Type*>{word, word, word, word});
    auto* function = batch.Context().construct<LLVMBC::Function>(type, 0, module);
    std::array<std::uint32_t, 6> inputs{0, 1, 0xFFFFFFFF, 0x12345678, 0xFEDCBA98, 0x80000000};
    for (auto high : inputs) {
        for (auto low : inputs) {
            for (auto offset : {0u, 1u, 2u, 3u, 4u, 7u, 255u, UINT32_MAX}) {
                auto* instruction = batch.Context().construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{batch.Constant(32, 332), batch.Constant(32, high), batch.Constant(32, low), batch.Constant(32, offset)});
                auto source = (static_cast<std::uint64_t>(high) << 32) | low;
                batch.Expect(*instruction, static_cast<std::uint32_t>(source >> ((offset % 4) * 8)));
            }
        }
    }
    auto* badOpcode = batch.Context().construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{batch.Constant(32, 333), batch.Constant(32, 0), batch.Constant(32, 0), batch.Constant(32, 0)});
    Fixtures::Reject<std::runtime_error>([&] { batch.Lowering().Get(*badOpcode); });
    auto* badCount = batch.Context().construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{batch.Constant(32, 332)});
    Fixtures::Reject<std::runtime_error>([&] { batch.Lowering().Get(*badCount); });
    batch.Verify();
}
