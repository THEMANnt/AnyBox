#pragma once

#include "Scalar.hpp"
#include "Buffer.hpp"
#include "Local.hpp"
#include <function.hpp>
#include <vector>

namespace AnyBox::Shader::Dxil {
class FunctionLowering {
public:
    explicit FunctionLowering(ShaderRecompiler::IrProgram& program);
    FunctionLowering(ShaderRecompiler::IrProgram& program, const ResourceLayout& layout, IResourceBindings& bindings);
    void Run(const LLVMBC::Function& function);

private:
    using IrBlock = ShaderRecompiler::IrBlock;
    using IrValue = ShaderRecompiler::IrValue;
    struct Edge {
        const LLVMBC::BasicBlock* source;
        const LLVMBC::BasicBlock* target;
        IrBlock* block;
    };
    struct Phi {
        const LLVMBC::PHINode* source;
        const LLVMBC::BasicBlock* block;
        IrValue* value;
    };
    IrBlock& createBlock();
    void prepare(const LLVMBC::Function& function);
    void emitBlock(const LLVMBC::BasicBlock& source);
    void emitTerminator(const LLVMBC::BasicBlock& source, const LLVMBC::Instruction& instruction);
    void emitSwitch(const LLVMBC::BasicBlock& source, const LLVMBC::SwitchInst& instruction);
    void branch(IrBlock& source, IrBlock& target);
    void conditional(IrBlock& source, IrValue& condition, IrBlock& trueTarget, IrBlock& falseTarget);
    void connectPhis();
    void orderBlocks();
    ShaderRecompiler::IrProgram& program;
    ShaderRecompiler::IrBuilder builder;
    ScalarLowering scalar;
    LocalLowering locals;
    std::unique_ptr<BufferLowering> buffers;
    std::unordered_map<const LLVMBC::BasicBlock*, IrBlock*> blocks;
    std::vector<const LLVMBC::BasicBlock*> order;
    std::vector<Edge> edges;
    std::vector<Phi> phis;
};
}
