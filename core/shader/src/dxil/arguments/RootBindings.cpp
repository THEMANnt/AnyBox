#include "RootBindings.hpp"
#include "Read.hpp"
#include <stdexcept>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

BufferAddress RootBindings::ResolveBuffer(const BufferBinding& binding, IrValue& index, bool nonUniform, IrBuilder& builder) {
    static_cast<void>(nonUniform);
    const auto& range = plan.Find(binding);
    if (range.kind == RootArgumentKind::Constants) throw std::logic_error("Inline root constants have no buffer address");
    return ReadBufferArguments(binding, range.firstUserRegister, index, builder);
}

ConstantBufferBinding RootBindings::ResolveConstantBuffer(const BufferBinding& binding, IrValue& index, bool nonUniform, IrBuilder& builder) {
    const auto& range = plan.Find(binding);
    if (range.kind != RootArgumentKind::Constants) return ResolveBuffer(binding, index, nonUniform, builder);
    if (binding.count != 1 || binding.resourceClass != ResourceClass::ConstantBuffer || index.Type() != IrType::U32) throw std::logic_error("Inline root constant binding is invalid");
    if (index.HasImmediate() && index.ImmediateU32() != binding.firstRegister) throw std::invalid_argument("Inline root constant resource index is outside its declared range");
    InlineConstants constants;
    for (unsigned word = 0; word < range.wordCount; ++word) {
        auto& value = builder.GetUserData(static_cast<ScalarReg>(range.firstUserRegister + word));
        auto* selected = &value;
        if (!index.HasImmediate()) selected = &builder.Select(builder.IEqual(index, builder.Constant(binding.firstRegister)), value, builder.Constant(0));
        constants.words.push_back(selected);
    }
    return constants;
}
