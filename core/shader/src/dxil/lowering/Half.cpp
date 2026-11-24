#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::legacyHalf(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 2 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL half-to-float conversion has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || width(*instruction.getArgOperand(1)) != 32) throw std::runtime_error("DXIL half-to-float conversion requires 32-bit integer operands");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 131) throw std::runtime_error("DXIL half-to-float conversion opcode is invalid");
    auto& half = builder.BitCastF16(Get(*instruction.getArgOperand(1)));
    return builder.Emit(IrOpcode::ConvertF32F16, IrType::F32, {&half});
}
