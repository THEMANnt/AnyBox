#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::compare(const LLVMBC::ICmpInst& instruction) {
    using Predicate = LLVMBC::Instruction;
    auto* leftSource = instruction.getOperand(0);
    auto bits = width(*leftSource);
    if (width(*instruction.getOperand(1)) != bits) throw std::runtime_error("DXIL comparison operand widths do not match");
    if (bits == 64) return wideCompare(instruction);
    auto predicate = instruction.getPredicate();
    bool isSigned = predicate >= Predicate::ICMP_SGT && predicate <= Predicate::ICMP_SLE;
    auto& left = isSigned ? signedValue(Get(*leftSource), bits) : unsignedValue(Get(*leftSource));
    auto& rightValue = Get(*instruction.getOperand(1));
    auto& right = isSigned ? signedValue(rightValue, bits) : unsignedValue(rightValue);
    IrOpcode opcode;
    switch (predicate) {
        case Predicate::ICMP_EQ: opcode = IrOpcode::IEqual32; break;
        case Predicate::ICMP_NE: opcode = IrOpcode::INotEqual32; break;
        case Predicate::ICMP_UGT: opcode = IrOpcode::UGreaterThan32; break;
        case Predicate::ICMP_UGE: opcode = IrOpcode::UGreaterThanEqual32; break;
        case Predicate::ICMP_ULT: opcode = IrOpcode::ULessThan32; break;
        case Predicate::ICMP_ULE: opcode = IrOpcode::ULessThanEqual32; break;
        case Predicate::ICMP_SGT: opcode = IrOpcode::SGreaterThan32; break;
        case Predicate::ICMP_SGE: opcode = IrOpcode::SGreaterThanEqual32; break;
        case Predicate::ICMP_SLT: opcode = IrOpcode::SLessThan32; break;
        case Predicate::ICMP_SLE: opcode = IrOpcode::SLessThanEqual32; break;
        default: throw std::runtime_error("DXIL integer comparison predicate is invalid");
    }
    return builder.Emit(opcode, IrType::Bool, {&left, &right});
}
