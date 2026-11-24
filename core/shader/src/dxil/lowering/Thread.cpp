#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::thread(const LLVMBC::CallInst& instruction, unsigned opcode) {
    auto count = opcode == 96 ? 1u : 2u;
    if (instruction.getNumArgOperands() != count || width(instruction) != 32) throw std::runtime_error("DXIL thread intrinsic has an invalid signature");
    auto constant = [&](unsigned index) {
        auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
        if (!value || width(*value) != 32) throw std::runtime_error("DXIL thread intrinsic requires a constant 32-bit operand");
        return value->getUniqueInteger().getZExtValue();
    };
    if (constant(0) != opcode) throw std::runtime_error("DXIL thread intrinsic opcode does not match its declaration");
    auto component = count == 1 ? 0 : constant(1);
    if (component >= 3) throw std::runtime_error("DXIL thread intrinsic component is out of range");
    StageInputKind kind;
    switch (opcode) {
        case 93: kind = StageInputKind::GlobalInvocationId; break;
        case 94: kind = StageInputKind::WorkgroupId; break;
        case 95: kind = StageInputKind::LocalInvocationId; break;
        case 96: kind = StageInputKind::LocalInvocationIndex; break;
        default: throw std::runtime_error("DXIL thread intrinsic opcode is not implemented");
    }
    return builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<std::uint32_t>(kind)), &builder.Constant(static_cast<std::uint32_t>(component))});
}
