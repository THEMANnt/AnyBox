#include "Function.hpp"
#include <cast.hpp>
#include <algorithm>
#include <unordered_set>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::FunctionLowering;

FunctionLowering::FunctionLowering(IrProgram& program) : program(program), builder(program), scalar(program, builder), locals(program, builder) { scalar.SetLocals(locals); }

FunctionLowering::FunctionLowering(IrProgram& program, const ResourceLayout& layout, IResourceBindings& bindings) : FunctionLowering(program) {
    buffers = std::make_unique<BufferLowering>(program, builder, layout, bindings);
    scalar.SetBuffers(*buffers);
}

IrBlock& FunctionLowering::createBlock() {
    auto& block = program.CreateBlock();
    program.BlockOrder().push_back(&block);
    BlockInfo info;
    info.id = block.Id();
    program.Metadata().blockInfo.push_back(std::move(info));
    return block;
}

void FunctionLowering::prepare(const LLVMBC::Function& function) {
    if (function.empty()) throw std::runtime_error("DXIL function has no implementation");
    if (function.getFunctionType()->getNumParams() != 0 || function.getFunctionType()->getReturnType()->getTypeID() != LLVMBC::Type::TypeID::VoidTyID) throw std::runtime_error("DXIL function arguments and returned values are not implemented");
    std::unordered_set<const LLVMBC::BasicBlock*> members;
    for (auto& block : function) members.insert(&block);
    std::unordered_set<const LLVMBC::BasicBlock*> visited;
    std::vector<std::pair<const LLVMBC::BasicBlock*, bool>> pending{{&function.getEntryBlock(), false}};
    while (!pending.empty()) {
        auto [block, complete] = pending.back();
        pending.pop_back();
        if (!members.contains(block)) throw std::runtime_error("DXIL branch targets a foreign basic block");
        if (complete) { order.push_back(block); continue; }
        if (!visited.insert(block).second) continue;
        pending.emplace_back(block, true);
        auto* terminator = block->getTerminator();
        if (!terminator) throw std::runtime_error("DXIL block has no terminator");
        if (auto* branch = LLVMBC::dyn_cast<LLVMBC::BranchInst>(terminator)) {
            for (unsigned index = 0; index < branch->getNumSuccessors(); ++index) pending.emplace_back(branch->getSuccessor(index), false);
        } else if (auto* selection = LLVMBC::dyn_cast<LLVMBC::SwitchInst>(terminator)) {
            pending.emplace_back(selection->getDefaultDest(), false);
            for (auto entry = selection->case_begin(); entry != selection->case_end(); ++entry) pending.emplace_back(entry->getCaseSuccessor(), false);
        } else if (!LLVMBC::isa<LLVMBC::ReturnInst>(terminator) && !LLVMBC::isa<LLVMBC::UnreachableInst>(terminator)) {
            throw std::runtime_error("DXIL terminator is not implemented");
        }
    }
    if (visited.size() != members.size()) throw std::runtime_error("DXIL unreachable basic blocks are not implemented");
    std::ranges::reverse(order);
    for (auto* source : order) blocks.emplace(source, &createBlock());
    program.SetEntryBlock(*blocks.at(&function.getEntryBlock()));
}

void FunctionLowering::emitBlock(const LLVMBC::BasicBlock& source) {
    builder.SetInsertionPoint(*blocks.at(&source));
    bool nonPhi = false;
    bool terminated = false;
    for (auto& instruction : source) {
        if (terminated) throw std::runtime_error("DXIL instruction follows a block terminator");
        if (auto* phi = LLVMBC::dyn_cast<LLVMBC::PHINode>(&instruction)) {
            if (nonPhi) throw std::runtime_error("DXIL PHI follows a non-PHI instruction");
            auto& value = builder.Emit(IrOpcode::Phi, ScalarLowering::GetType(*phi), {});
            scalar.Bind(*phi, value);
            phis.push_back({phi, &source, &value});
        } else {
            nonPhi = true;
            if (instruction.isTerminator()) {
                emitTerminator(source, instruction);
                terminated = true;
            } else {
                scalar.Emit(instruction);
            }
        }
    }
    if (!terminated) throw std::runtime_error("DXIL basic block does not end with a terminator");
}

void FunctionLowering::Run(const LLVMBC::Function& function) {
    if (!program.Blocks().empty() || !blocks.empty()) throw std::logic_error("DXIL function lowering requires an empty program");
    prepare(function);
    locals.Prepare(function);
    for (auto* source : order) emitBlock(*source);
    connectPhis();
    orderBlocks();
    ValidateProgram(program, true);
}
