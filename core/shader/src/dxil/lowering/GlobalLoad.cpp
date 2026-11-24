#include "Scalar.hpp"
#include <cast.hpp>
#include <array>
#include <limits>

using namespace ShaderRecompiler;
using AnyBox::Shader::Dxil::ScalarLowering;

IrValue& ScalarLowering::globalLoad(const LLVMBC::CallInst& instruction, unsigned words) {
    if (words < 1 || words > 4 || instruction.getNumArgOperands() != 4) throw std::runtime_error("Xbox global load has an invalid signature");
    auto* result = instruction.getType();
    if (words == 1) {
        if (width(instruction) != 32) throw std::runtime_error("Xbox global load must return a 32-bit word");
    } else {
        if (!result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != words) throw std::runtime_error("Xbox global load must return a structure of words");
        for (unsigned index = 0; index < words; ++index) {
            auto* member = result->getStructElementType(index);
            if (!member->isIntegerTy() || member->getIntegerBitWidth() != 32) throw std::runtime_error("Xbox global load result contains a non-word member");
        }
    }
    auto* opcode = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(0));
    if (!opcode || width(*opcode) != 32 || opcode->getUniqueInteger().getZExtValue() != 355 + words) throw std::runtime_error("Xbox global load has an invalid opcode");
    auto* input = instruction.getArgOperand(1);
    if (width(*input) != 64) throw std::runtime_error("Xbox global load requires a 64-bit guest address");
    for (unsigned index = 2; index < 4; ++index) {
        auto* control = instruction.getArgOperand(index);
        if (width(*control) != 32 || !LLVMBC::isa<LLVMBC::UndefValue>(control)) throw std::runtime_error("Xbox explicit global load cache controls are not implemented");
    }

    auto& address = Get(*input);
    auto& low = builder.CompositeExtract(address, 0);
    auto& high = builder.CompositeExtract(address, 1);
    auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    memory.dataDwords = words;
    memory.componentCount = words;
    auto& metadata = program.Resources().memoryInfo;
    if (metadata.size() >= std::numeric_limits<std::uint32_t>::max()) throw std::length_error("DXIL memory operation count exceeds the IR limit");
    MemoryFlags flags{static_cast<std::uint32_t>(metadata.size()), 0};
    metadata.push_back(memory);
    constexpr std::array opcodes{IrOpcode::LoadAddressU32, IrOpcode::LoadAddressU32x2, IrOpcode::LoadAddressU32x3, IrOpcode::LoadAddressU32x4};
    auto operation = opcodes.at(words - 1);
    return builder.Emit(operation, IrOpcodeType(operation), {&resource, &low, &high, &builder.ConstantBool(true)}, flags);
}
