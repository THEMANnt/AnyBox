#include "Builder.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::Flow::Builder;

IrValue& Builder::constant(const IrValue& value) {
    if (auto found = constants.find(&value); found != constants.end()) return *found->second;
    auto& copy = output.CreateValue(value.Opcode(), value.Type(), value.Flags<std::uint64_t>());
    switch (value.Type()) {
        case IrType::Bool: copy.SetImmediateBool(value.ImmediateBool()); break;
        case IrType::U8: copy.SetImmediateU8(value.ImmediateU8()); break;
        case IrType::U16: copy.SetImmediateU16(value.ImmediateU16()); break;
        case IrType::U32: copy.SetImmediateU32(value.ImmediateU32()); break;
        case IrType::U64: copy.SetImmediateU64(value.ImmediateU64()); break;
        case IrType::F16: copy.SetImmediateF16Bits(value.ImmediateF16Bits()); break;
        case IrType::F32: copy.SetImmediateF32(value.ImmediateF32()); break;
        case IrType::ScalarReg:
        case IrType::VectorReg: copy.SetRegister(value.Register()); break;
        default: throw std::runtime_error("DXIL structured constant type is not implemented");
    }
    constants.emplace(&value, &copy);
    return copy;
}

IrValue& Builder::operand(const IrValue& input, State& state) {
    auto* value = input.Resolve();
    if (value->HasImmediate() || value->Type() == IrType::ScalarReg || value->Type() == IrType::VectorReg) return constant(*value);
    if (state.origin && value->Parent() == state.origin->block && !value->IsPhi()) return *state.definitions.at(value);
    return entry(*value, state);
}
