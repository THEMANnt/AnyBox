#include "Scalar.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::wideCompare(const LLVMBC::ICmpInst& instruction) {
    using Predicate = LLVMBC::Instruction;
    auto* left = &Get(*instruction.getOperand(0));
    auto* right = &Get(*instruction.getOperand(1));
    auto predicate = instruction.getPredicate();
    IrOpcode opcode;
    bool invert = false;
    switch (predicate) {
        case Predicate::ICMP_EQ: opcode = IrOpcode::IEqual64; break;
        case Predicate::ICMP_NE: opcode = IrOpcode::INotEqual64; break;
        case Predicate::ICMP_UGT: opcode = IrOpcode::UGreaterThan64; break;
        case Predicate::ICMP_UGE: opcode = IrOpcode::ULessThan64; invert = true; break;
        case Predicate::ICMP_ULT: opcode = IrOpcode::ULessThan64; break;
        case Predicate::ICMP_ULE: opcode = IrOpcode::UGreaterThan64; invert = true; break;
        case Predicate::ICMP_SGT: opcode = IrOpcode::SLessThan64; std::swap(left, right); break;
        case Predicate::ICMP_SGE: opcode = IrOpcode::SLessThan64; invert = true; break;
        case Predicate::ICMP_SLT: opcode = IrOpcode::SLessThan64; break;
        case Predicate::ICMP_SLE: opcode = IrOpcode::SLessThan64; std::swap(left, right); invert = true; break;
        default: throw std::runtime_error("DXIL wide integer comparison predicate is invalid");
    }
    auto& result = builder.Emit(opcode, IrType::Bool, {left, right});
    return invert ? builder.LogicalNot(result) : result;
}
