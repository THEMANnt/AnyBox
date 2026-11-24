#include "Buffer.hpp"
#include <algorithm>
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::BufferLowering;

namespace {
IrValue& Word(IrBuilder& builder, std::span<IrValue* const> words, IrValue& offset, std::uint32_t byteSize) {
    if (offset.HasImmediate()) {
        auto value = offset.ImmediateU64();
        if (value > byteSize || byteSize - value < 4) return builder.Constant(0);
        return *words[value / 4];
    }
    auto* result = &builder.Constant(0);
    auto count = std::min(words.size(), static_cast<std::size_t>(byteSize / 4));
    for (std::size_t index = 0; index < count; ++index) {
        auto& selected = builder.Emit(IrOpcode::IEqual64, IrType::Bool, {&offset, &builder.ConstantU64(index * 4)});
        result = &builder.Select(selected, *words[index], *result);
    }
    return *result;
}
}

void BufferLowering::constantWords(const LLVMBC::CallInst& instruction, std::span<IrValue* const> words, IrValue& row, unsigned bits, std::uint32_t byteSize, bool floating) {
    if (row.Type() != IrType::U32 || words.size() * 4 < byteSize) throw std::logic_error("Inline constant buffer word layout is invalid");
    for (auto* word : words) if (!word || word->Type() != IrType::U32) throw std::logic_error("Inline constant buffer contains a non-word value");
    auto& index = builder.ConstructU64(row, builder.Constant(0));
    auto& base = row.HasImmediate() ? builder.ConstantU64(static_cast<std::uint64_t>(row.ImmediateU32()) * 16) : builder.Emit(IrOpcode::ShiftLeftLogical64, IrType::U64, {&index, &builder.Constant(4)});
    std::array<IrValue*, 5> output{};
    for (unsigned component = 0; component < 128 / bits; ++component) {
        auto displacement = component * (bits / 8);
        auto& offset = base.HasImmediate() ? builder.ConstantU64(base.ImmediateU64() + displacement) : builder.Emit(IrOpcode::IAdd64, IrType::U64, {&base, &builder.ConstantU64(displacement)});
        auto* value = &Word(builder, words, offset, byteSize);
        if (bits == 64) {
            auto& highOffset = offset.HasImmediate() ? builder.ConstantU64(offset.ImmediateU64() + 4) : builder.Emit(IrOpcode::IAdd64, IrType::U64, {&offset, &builder.ConstantU64(4)});
            auto& joined = builder.ConstructU64(*value, Word(builder, words, highOffset, byteSize));
            auto& end = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&offset, &builder.ConstantU64(8)});
            auto& outside = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {&end, &builder.ConstantU64(byteSize)});
            value = &builder.Select(builder.LogicalNot(outside), joined, builder.ConstantU64(0));
        }
        output[component] = floating ? &builder.BitCastF32(*value) : value;
    }
    aggregates.emplace(&instruction, output);
}
