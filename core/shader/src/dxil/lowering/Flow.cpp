#include "Function.hpp"
#include <cast.hpp>
#include <unordered_set>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::FunctionLowering;

void FunctionLowering::branch(IrBlock& source, IrBlock& target) {
    builder.SetInsertionPoint(source);
    builder.Branch(target);
    auto& info = program.Metadata().blockInfo.at(source.Id());
    info.terminator.kind = TerminatorKind::Branch;
    info.terminator.trueBlock = target.Id();
}

void FunctionLowering::conditional(IrBlock& source, IrValue& condition, IrBlock& trueTarget, IrBlock& falseTarget) {
    if (condition.Type() != IrType::Bool) throw std::runtime_error("DXIL branch requires a scalar boolean condition");
    if (&trueTarget == &falseTarget) { branch(source, trueTarget); return; }
    builder.SetInsertionPoint(source);
    builder.BranchConditional(condition, trueTarget, falseTarget);
    auto& info = program.Metadata().blockInfo.at(source.Id());
    info.condition = &condition;
    info.terminator.kind = TerminatorKind::ConditionalBranch;
    info.terminator.condition = BranchCondition::ScalarInstruction;
    info.terminator.trueBlock = trueTarget.Id();
    info.terminator.falseBlock = falseTarget.Id();
}

void FunctionLowering::emitTerminator(const LLVMBC::BasicBlock& source, const LLVMBC::Instruction& instruction) {
    auto& output = *blocks.at(&source);
    if (auto* input = LLVMBC::dyn_cast<LLVMBC::BranchInst>(&instruction)) {
        auto* first = input->getSuccessor(0);
        if (input->isConditional()) {
            auto* second = input->getSuccessor(1);
            conditional(output, scalar.Get(*input->getCondition()), *blocks.at(first), *blocks.at(second));
            if (first != second) edges.push_back({&source, second, &output});
        } else {
            branch(output, *blocks.at(first));
        }
        edges.push_back({&source, first, &output});
    } else if (auto* selection = LLVMBC::dyn_cast<LLVMBC::SwitchInst>(&instruction)) {
        emitSwitch(source, *selection);
    } else if (auto* returned = LLVMBC::dyn_cast<LLVMBC::ReturnInst>(&instruction)) {
        if (returned->getReturnValue()) throw std::runtime_error("DXIL non-void return is not implemented");
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    } else if (LLVMBC::isa<LLVMBC::UnreachableInst>(&instruction)) {
        (void)builder.Emit(IrOpcode::Unreachable, IrType::Void, {});
    } else {
        throw std::runtime_error("DXIL terminator is not implemented");
    }
}

void FunctionLowering::emitSwitch(const LLVMBC::BasicBlock& source, const LLVMBC::SwitchInst& instruction) {
    auto& selector = scalar.Get(*instruction.getCondition());
    auto* current = blocks.at(&source);
    std::unordered_set<std::uint64_t> cases;
    for (auto entry = instruction.case_begin(); entry != instruction.case_end(); ++entry) {
        auto* key = entry->getCaseValue();
        if (key->getType() != instruction.getCondition()->getType()) throw std::runtime_error("DXIL switch case type does not match its selector");
        if (!cases.insert(key->getUniqueInteger().getZExtValue()).second) throw std::runtime_error("DXIL switch case value is duplicated");
        auto& next = createBlock();
        builder.SetInsertionPoint(*current);
        auto& value = scalar.Get(*key);
        auto opcode = selector.Type() == IrType::Bool ? IrOpcode::LogicalXor : selector.Type() == IrType::U64 ? IrOpcode::IEqual64 : IrOpcode::IEqual32;
        auto& condition = builder.Emit(opcode, IrType::Bool, {&selector, &value});
        auto& equal = selector.Type() == IrType::Bool ? builder.LogicalNot(condition) : condition;
        conditional(*current, equal, *blocks.at(entry->getCaseSuccessor()), next);
        edges.push_back({&source, entry->getCaseSuccessor(), current});
        current = &next;
    }
    branch(*current, *blocks.at(instruction.getDefaultDest()));
    edges.push_back({&source, instruction.getDefaultDest(), current});
}
