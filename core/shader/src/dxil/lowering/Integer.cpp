#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::divide(IrValue& left, IrValue& right, bool isSigned, bool remainder, unsigned bits) {
    if (!isSigned) {
        auto& quotient = builder.Emit(IrOpcode::UDiv32, IrType::U32, {&left, &right});
        return remainder ? builder.ISub(left, builder.IMul(quotient, right)) : quotient;
    }
    auto& signedLeft = signedValue(left, bits);
    auto& signedRight = signedValue(right, bits);
    auto& zero = builder.Constant(0);
    auto& negativeLeft = builder.Emit(IrOpcode::SLessThan32, IrType::Bool, {&signedLeft, &zero});
    auto& negativeRight = builder.Emit(IrOpcode::SLessThan32, IrType::Bool, {&signedRight, &zero});
    auto& absoluteLeft = builder.Select(negativeLeft, builder.ISub(zero, signedLeft), signedLeft);
    auto& absoluteRight = builder.Select(negativeRight, builder.ISub(zero, signedRight), signedRight);
    auto& quotient = builder.Emit(IrOpcode::UDiv32, IrType::U32, {&absoluteLeft, &absoluteRight});
    auto& negativeQuotient = builder.Emit(IrOpcode::LogicalXor, IrType::Bool, {&negativeLeft, &negativeRight});
    auto& signedQuotient = builder.Select(negativeQuotient, builder.ISub(zero, quotient), quotient);
    return remainder ? builder.ISub(signedLeft, builder.IMul(signedQuotient, signedRight)) : signedQuotient;
}

IrValue& ScalarLowering::binary(const LLVMBC::BinaryOperator& instruction) {
    using Op = LLVMBC::Instruction;
    auto bits = width(instruction);
    if (width(*instruction.getOperand(0)) != bits || width(*instruction.getOperand(1)) != bits) throw std::runtime_error("DXIL binary integer operand widths do not match");
    if (bits == 64) return wideBinary(instruction);
    auto& left = unsignedValue(Get(*instruction.getOperand(0)));
    auto& right = unsignedValue(Get(*instruction.getOperand(1)));
    IrValue* output = nullptr;
    switch (instruction.getOpcode()) {
        case Op::Add: output = &builder.IAdd(left, right); break;
        case Op::Sub: output = &builder.ISub(left, right); break;
        case Op::Mul: output = &builder.IMul(left, right); break;
        case Op::And: output = &builder.BitwiseAnd(left, right); break;
        case Op::Or: output = &builder.BitwiseOr(left, right); break;
        case Op::Xor: output = &builder.BitwiseXor(left, right); break;
        case Op::Shl: output = &builder.ShiftLeftLogical(left, right); break;
        case Op::LShr: output = &builder.ShiftRightLogical(left, right); break;
        case Op::AShr: output = &builder.ShiftRightArithmetic(signedValue(left, bits), right); break;
        case Op::UDiv: output = &divide(left, right, false, false, bits); break;
        case Op::SDiv: output = &divide(left, right, true, false, bits); break;
        case Op::URem: output = &divide(left, right, false, true, bits); break;
        case Op::SRem: output = &divide(left, right, true, true, bits); break;
        default: throw std::runtime_error("DXIL binary operation is not implemented: " + std::to_string(instruction.getOpcode()));
    }
    return narrow(*output, bits);
}
