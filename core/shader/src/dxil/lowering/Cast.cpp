#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::cast(const LLVMBC::CastInst& instruction) {
    auto targetBits = width(instruction);
    auto* operand = instruction.getOperand(0);
    auto sourceBits = width(*operand);
    auto& value = Get(*operand);
    switch (instruction.getOpcode()) {
        case LLVMBC::Instruction::Trunc:
            if (targetBits >= sourceBits) throw std::runtime_error("DXIL truncation does not reduce integer width");
            return narrow(value, targetBits);
        case LLVMBC::Instruction::ZExt:
            if (targetBits <= sourceBits) throw std::runtime_error("DXIL zero extension does not increase integer width");
            if (targetBits == 64) return builder.ConstructU64(unsignedValue(value), builder.Constant(0));
            return unsignedValue(value);
        case LLVMBC::Instruction::SExt:
            if (targetBits <= sourceBits) throw std::runtime_error("DXIL sign extension does not increase integer width");
            if (targetBits == 64) {
                auto& low = signedValue(value, sourceBits);
                return builder.ConstructU64(low, builder.ShiftRightArithmetic(low, builder.Constant(31)));
            }
            return narrow(signedValue(value, sourceBits), targetBits);
        case LLVMBC::Instruction::BitCast:
            if (targetBits != sourceBits) throw std::runtime_error("DXIL integer bitcast changes bit width");
            return value;
        default: throw std::runtime_error("DXIL scalar cast is not implemented: " + std::to_string(instruction.getOpcode()));
    }
}
