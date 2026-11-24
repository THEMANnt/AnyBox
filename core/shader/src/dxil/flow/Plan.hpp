#pragma once

#include <IntermediateRepresentation/IrProgram.hpp>
#include <unordered_map>

namespace AnyBox::Shader::Dxil::Flow {
struct Assignment {
    const ShaderRecompiler::IrValue* target;
    const ShaderRecompiler::IrValue* source;
};

struct Origin {
    const ShaderRecompiler::IrBlock* block = nullptr;
    const ShaderRecompiler::IrValue* condition = nullptr;
    std::vector<Assignment> assignments;
};

class Plan {
public:
    explicit Plan(const ShaderRecompiler::IrProgram& source);
    ShaderRecompiler::ControlFlowGraph graph;
    std::vector<Origin> origins;

private:
    void splitPhiEdges(const ShaderRecompiler::IrProgram& source);
    std::unordered_map<const ShaderRecompiler::IrBlock*, std::uint32_t> ids;
};
}
