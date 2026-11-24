#include "Scalar.hpp"

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::floatCompare(const LLVMBC::FCmpInst& instruction) {
    using Predicate = LLVMBC::Instruction;
    auto& left = Get(*instruction.getOperand(0));
    auto& right = Get(*instruction.getOperand(1));
    if (left.Type() != IrType::F32 || right.Type() != IrType::F32) throw std::runtime_error("DXIL floating comparison requires float32 operands");
    IrOpcode opcode;
    switch (instruction.getPredicate()) {
        case Predicate::FCMP_FALSE: return builder.ConstantBool(false);
        case Predicate::FCMP_TRUE: return builder.ConstantBool(true);
        case Predicate::FCMP_OEQ: opcode = IrOpcode::FPOrdEqual32; break;
        case Predicate::FCMP_OGT: opcode = IrOpcode::FPOrdGreaterThan32; break;
        case Predicate::FCMP_OGE: opcode = IrOpcode::FPOrdGreaterThanEqual32; break;
        case Predicate::FCMP_OLT: opcode = IrOpcode::FPOrdLessThan32; break;
        case Predicate::FCMP_OLE: opcode = IrOpcode::FPOrdLessThanEqual32; break;
        case Predicate::FCMP_ONE: opcode = IrOpcode::FPOrdNotEqual32; break;
        case Predicate::FCMP_UEQ: opcode = IrOpcode::FPUnordEqual32; break;
        case Predicate::FCMP_UGT: opcode = IrOpcode::FPUnordGreaterThan32; break;
        case Predicate::FCMP_UGE: opcode = IrOpcode::FPUnordGreaterThanEqual32; break;
        case Predicate::FCMP_ULT: opcode = IrOpcode::FPUnordLessThan32; break;
        case Predicate::FCMP_ULE: opcode = IrOpcode::FPUnordLessThanEqual32; break;
        case Predicate::FCMP_UNE: opcode = IrOpcode::FPUnordNotEqual32; break;
        case Predicate::FCMP_ORD:
        case Predicate::FCMP_UNO: {
            auto& leftNan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&left});
            auto& rightNan = builder.Emit(IrOpcode::FPIsNan32, IrType::Bool, {&right});
            auto& unordered = builder.LogicalOr(leftNan, rightNan);
            return instruction.getPredicate() == Predicate::FCMP_UNO ? unordered : builder.LogicalNot(unordered);
        }
        default: throw std::runtime_error("DXIL floating comparison predicate is invalid");
    }
    return builder.Emit(opcode, IrType::Bool, {&left, &right});
}
