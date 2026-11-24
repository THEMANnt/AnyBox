#include "Bindings.hpp"
#include "Read.hpp"
#include <RuntimeAbi.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

ArgumentBindings::ArgumentBindings(const BufferArgumentPlan& plan) : plan(plan) {
    static_assert(BufferArgumentPlan::UserRegisterCapacity == RuntimeAbi::UserDataCapacity);
}

BufferAddress ArgumentBindings::ResolveBuffer(const BufferBinding& binding, IrValue& index, bool nonUniform, IrBuilder& builder) {
    static_cast<void>(nonUniform);
    return ReadBufferArguments(binding, plan.Find(binding).firstUserRegister, index, builder);
}
