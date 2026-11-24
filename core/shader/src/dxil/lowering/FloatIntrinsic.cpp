#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

namespace {
IrValue& Flush(IrBuilder& builder, IrValue& value) {
    auto& bits = builder.BitCastU32(value);
    auto& exponent = builder.BitwiseAnd(bits, builder.Constant(0x7F800000U));
    auto& zeroExponent = builder.IEqual(exponent, builder.Constant(0));
    auto& sign = builder.BitwiseAnd(bits, builder.Constant(0x80000000U));
    return builder.BitCastF32(builder.Select(zeroExponent, sign, bits));
}
}

IrValue& ScalarLowering::floatBinaryIntrinsic(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 3 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL floating binary intrinsic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32 || GetType(*instruction.getArgOperand(1)) != IrType::F32 || GetType(*instruction.getArgOperand(2)) != IrType::F32) throw std::runtime_error("DXIL floating binary intrinsic has invalid operand types");
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL floating binary intrinsic opcode must be constant");
    IrOpcode operation;
    switch (opcode->getUniqueInteger().getZExtValue()) {
        case 35: operation = IrOpcode::FPMax32; break;
        case 36: operation = IrOpcode::FPMin32; break;
        default: throw std::runtime_error("DXIL floating binary intrinsic opcode is not implemented");
    }
    auto& left = Flush(builder, Get(*instruction.getArgOperand(1)));
    auto& right = Flush(builder, Get(*instruction.getArgOperand(2)));
    return builder.Emit(operation, IrType::F32, {&left, &right});
}

IrValue& ScalarLowering::floatTernaryIntrinsic(const LLVMBC::CallInst& instruction) {
    if (instruction.getNumArgOperands() != 4 || GetType(instruction) != IrType::F32) throw std::runtime_error("DXIL floating ternary intrinsic has an invalid signature");
    if (width(*instruction.getArgOperand(0)) != 32) throw std::runtime_error("DXIL floating ternary intrinsic requires a 32-bit opcode");
    for (unsigned index = 1; index < 4; ++index) {
        if (GetType(*instruction.getArgOperand(index)) != IrType::F32) throw std::runtime_error("DXIL floating ternary intrinsic requires 32-bit floating operands");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || opcode->getUniqueInteger().getZExtValue() != 46) throw std::runtime_error("DXIL floating ternary intrinsic opcode is not implemented");

    auto& left = Get(*instruction.getArgOperand(1));
    auto& right = Get(*instruction.getArgOperand(2));
    auto& addend = Get(*instruction.getArgOperand(3));
    return builder.Emit(IrOpcode::FPMad32, IrType::F32, {&left, &right, &addend});
}
