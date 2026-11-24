#include "Scalar.hpp"
#include <cast.hpp>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::bitField(const LLVMBC::CallInst& instruction, bool insert) {
    if (instruction.getNumArgOperands() != (insert ? 5u : 4u) || width(instruction) != 32) throw std::runtime_error("DXIL bit field intrinsic has an invalid signature");
    for (unsigned index = 0; index < instruction.getNumArgOperands(); ++index) {
        if (width(*instruction.getArgOperand(index)) != 32) throw std::runtime_error("DXIL bit field intrinsic requires 32-bit integers");
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode) throw std::runtime_error("DXIL bit field opcode must be constant");
    auto operation = opcode->getUniqueInteger().getZExtValue();
    if (insert ? operation != 53 : operation != 51 && operation != 52) throw std::runtime_error("DXIL integer intrinsic opcode is not implemented");

    auto& requested = builder.BitwiseAnd(Get(*instruction.getArgOperand(1)), builder.Constant(31));
    auto& offset = builder.BitwiseAnd(Get(*instruction.getArgOperand(2)), builder.Constant(31));
    auto& available = builder.ISub(builder.Constant(32), offset);
    auto& count = builder.Emit(IrOpcode::UMin32, IrType::U32, {&requested, &available});
    auto& source = Get(*instruction.getArgOperand(3));
    if (insert) {
        auto& destination = Get(*instruction.getArgOperand(4));
        return builder.Emit(IrOpcode::BitFieldInsert, IrType::U32, {&destination, &source, &offset, &count});
    }
    return builder.Emit(operation == 51 ? IrOpcode::BitFieldSExtract : IrOpcode::BitFieldUExtract, IrType::U32, {&source, &offset, &count});
}
