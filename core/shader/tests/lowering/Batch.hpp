#pragma once

#include "lowering/Scalar.hpp"
#include "Session.hpp"
#include <context.hpp>
#include <Optimization/ConstantFolder.hpp>
#include <array>
#include <string>
#include <vector>

namespace LoweringTests {
class Batch {
public:
    Batch() : builder(program), lowering(program, builder) {
        auto& block = program.CreateBlock();
        program.SetEntryBlock(block);
        program.BlockOrder().push_back(&block);
        builder.SetInsertionPoint(block);
    }
    LLVMBC::ConstantInt* Constant(unsigned bits, std::uint64_t value) {
        return LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(context, bits), value);
    }
    void Expect(const LLVMBC::Value& source, std::uint64_t expected) {
        checks.push_back({&lowering.Get(source), expected});
    }
    void Verify() {
        ShaderRecompiler::ConstantFolder{}.Fold(program);
        for (const auto& check : checks) {
            auto* value = check.value->Resolve();
            if (!value->HasImmediate()) throw std::runtime_error("Lowered integer expression did not fold to a constant");
            auto actual = value->Type() == ShaderRecompiler::IrType::Bool ? static_cast<std::uint64_t>(value->ImmediateBool()) : value->Type() == ShaderRecompiler::IrType::U64 ? value->ImmediateU64() : value->ImmediateU32();
            if (actual != check.expected) throw std::runtime_error("Lowered integer result mismatch: " + std::to_string(actual) + " != " + std::to_string(check.expected));
        }
    }
    LLVMBC::LLVMContext& Context() { return context; }
    AnyBox::Shader::Dxil::ScalarLowering& Lowering() { return lowering; }

private:
    struct Check {
        ShaderRecompiler::IrValue* value;
        std::uint64_t expected;
    };
    AnyBox::Shader::DxilSession session;
    LLVMBC::LLVMContext context;
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder;
    AnyBox::Shader::Dxil::ScalarLowering lowering;
    std::vector<Check> checks;
};

inline std::int64_t Signed(std::uint32_t value, unsigned bits) {
    auto sign = std::uint64_t{1} << (bits - 1);
    return (value & sign) != 0 ? static_cast<std::int64_t>(value) - static_cast<std::int64_t>(sign * 2) : value;
}

void CheckComparisonsAndCasts();
void CheckWideIntegers();
void CheckIntrinsics();
}
