#pragma once

#include "../lowering/Buffer.hpp"
#include <anybox/shader/RootArguments.hpp>

namespace AnyBox::Shader::Dxil {
class RootBindings : public IResourceBindings {
public:
    explicit RootBindings(const RootArgumentPlan& plan) : plan(plan) {}
    BufferAddress ResolveBuffer(const BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override;
    ConstantBufferBinding ResolveConstantBuffer(const BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override;

private:
    const RootArgumentPlan& plan;
};
}
