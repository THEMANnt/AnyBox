#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::wideBinary(const LLVMBC::BinaryOperator& instruction) {
    using Op = LLVMBC::Instruction;
    auto& left = Get(*instruction.getOperand(0));
    auto& right = Get(*instruction.getOperand(1));
    auto operation = instruction.getOpcode();
    if (operation == Op::Or || operation == Op::Xor) {
        auto opcode = operation == Op::Or ? IrOpcode::BitwiseOr32 : IrOpcode::BitwiseXor32;
        auto& low = builder.Emit(opcode, IrType::U32, {&builder.CompositeExtract(left, 0), &builder.CompositeExtract(right, 0)});
        auto& high = builder.Emit(opcode, IrType::U32, {&builder.CompositeExtract(left, 1), &builder.CompositeExtract(right, 1)});
        return builder.ConstructU64(low, high);
    }
    IrOpcode opcode;
    IrValue* argument = &right;
    switch (operation) {
        case Op::Add: opcode = IrOpcode::IAdd64; break;
        case Op::Sub: opcode = IrOpcode::ISub64; break;
        case Op::Mul: opcode = IrOpcode::IMul64; break;
        case Op::And: opcode = IrOpcode::BitwiseAnd64; break;
        case Op::Shl: opcode = IrOpcode::ShiftLeftLogical64; argument = &builder.CompositeExtract(right, 0); break;
        case Op::LShr: opcode = IrOpcode::ShiftRightLogical64; argument = &builder.CompositeExtract(right, 0); break;
        case Op::AShr: opcode = IrOpcode::ShiftRightArithmetic64; argument = &builder.CompositeExtract(right, 0); break;
        default: throw std::runtime_error("DXIL 64-bit binary operation is not implemented: " + std::to_string(operation));
    }
    return builder.Emit(opcode, IrType::U64, {&left, argument});
}
