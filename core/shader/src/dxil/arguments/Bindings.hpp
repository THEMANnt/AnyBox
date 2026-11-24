#pragma once

#include "../lowering/Buffer.hpp"
#include <anybox/shader/BufferArguments.hpp>

namespace AnyBox::Shader::Dxil {
class ArgumentBindings : public IResourceBindings {
public:
    explicit ArgumentBindings(const BufferArgumentPlan& plan);
    BufferAddress ResolveBuffer(const BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override;

private:
    const BufferArgumentPlan& plan;
};
}
