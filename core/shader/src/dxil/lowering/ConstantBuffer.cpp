#include "Buffer.hpp"
#include "Scalar.hpp"

using namespace ShaderRecompiler;
using namespace AnyBox::Shader;
using AnyBox::Shader::Dxil::BufferLowering;

void BufferLowering::constantBuffer(const LLVMBC::CallInst& instruction, unsigned bits, ScalarLowering& scalar, bool floating) {
    if (instruction.getNumArgOperands() != 3 || constant(instruction, 0, 32) != 59) throw std::runtime_error("DXIL legacy constant buffer load has an invalid signature");
    auto handle = handles.find(instruction.getArgOperand(1));
    if (handle == handles.end()) throw std::runtime_error("DXIL constant buffer load requires a previously created handle");
    const auto& binding = *handle->second.binding;
    if (binding.resourceClass != ResourceClass::ConstantBuffer || binding.kind != BufferKind::Constant) throw std::runtime_error("DXIL constant buffer load uses a non-CBV resource");
    auto count = 128 / bits;
    auto* result = instruction.getType();
    if (!result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != count) throw std::runtime_error("DXIL legacy constant buffer result has an invalid member count");
    for (unsigned index = 0; index < count; ++index) {
        auto* member = result->getStructElementType(index);
        bool valid = floating ? member->getTypeID() == LLVMBC::Type::TypeID::FloatTyID : member->isIntegerTy() && member->getIntegerBitWidth() == bits;
        if (!valid) throw std::runtime_error("DXIL legacy constant buffer result has an invalid member type");
    }
    auto* selected = instruction.getArgOperand(2);
    if (!selected->getType()->isIntegerTy() || selected->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("DXIL legacy constant buffer register index must be 32-bit");
    auto mapped = bindings.ResolveConstantBuffer(binding, scalar.Get(*handle->second.index), handle->second.nonUniform, builder);
    if (auto* words = std::get_if<InlineConstants>(&mapped)) {
        constantWords(instruction, words->words, scalar.Get(*selected), bits, binding.byteSize, floating);
        return;
    }
    auto view = std::get<BufferAddress>(mapped);
    if (view.address.Type() != IrType::U64 || view.byteLength.Type() != IrType::U64) throw std::logic_error("DXIL constant buffer binding requires a 64-bit address and size");
    auto& index = builder.ConstructU64(scalar.Get(*selected), builder.Constant(0));
    auto& base = builder.Emit(IrOpcode::ShiftLeftLogical64, IrType::U64, {&index, &builder.Constant(4)});
    std::array<IrValue*, 5> output{};
    for (unsigned component = 0; component < count; ++component) {
        auto& offset = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&base, &builder.ConstantU64(component * (bits / 8))});
        auto& end = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&offset, &builder.ConstantU64(bits / 8)});
        auto& outside = builder.Emit(IrOpcode::UGreaterThan64, IrType::Bool, {&end, &view.byteLength});
        auto& address = builder.Emit(IrOpcode::IAdd64, IrType::U64, {&view.address, &offset});
        auto& value = read(address, builder.LogicalNot(outside), bits, false);
        output[component] = floating ? &builder.BitCastF32(value) : &value;
    }
    aggregates.emplace(&instruction, output);
}
