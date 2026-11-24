#include "Read.hpp"
#include <anybox/shader/BufferArguments.hpp>
#include <stdexcept>

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

namespace {
BufferAddress Read(ShaderRecompiler::IrBuilder& builder, std::uint32_t first) {
    auto word = [&](std::uint32_t index) -> IrValue& { return builder.GetUserData(static_cast<ScalarReg>(first + index)); };
    return {builder.ConstructU64(word(0), word(1)), builder.ConstructU64(word(2), word(3))};
}
}

BufferAddress AnyBox::Shader::Dxil::ReadBufferArguments(const BufferBinding& binding, std::uint32_t first, IrValue& index, IrBuilder& builder) {
    if (index.Type() != IrType::U32) throw std::invalid_argument("Buffer argument selection must be a 32-bit unsigned index");
    if (index.HasImmediate()) {
        auto selected = index.ImmediateU32();
        if (selected < binding.firstRegister || selected - binding.firstRegister >= binding.count) throw std::invalid_argument("Buffer argument index is outside its declared range");
        return Read(builder, first + (selected - binding.firstRegister) * BufferArgumentPlan::WordsPerBuffer);
    }

    auto& relative = builder.ISub(index, builder.Constant(binding.firstRegister));
    auto* address = &builder.ConstantU64(0);
    auto* length = &builder.ConstantU64(0);
    for (std::uint32_t element = 0; element < binding.count; ++element) {
        auto value = Read(builder, first + element * BufferArgumentPlan::WordsPerBuffer);
        auto& selected = builder.IEqual(relative, builder.Constant(element));
        address = &builder.Select(selected, value.address, *address);
        length = &builder.Select(selected, value.byteLength, *length);
    }
    return {*address, *length};
}
