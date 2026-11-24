#include "ResourceLayout.hpp"
#include "DxilMetadata.hpp"
#include "DxilModule.hpp"
#include <function.hpp>
#include <algorithm>
#include <limits>
#include <string_view>

using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

ResourceLayout::ResourceLayout(const LLVMBC::Module& module) {
    auto* metadata = module.getNamedMetadata("dx.resources");
    if (!metadata) return;
    if (metadata->getNumOperands() != 1) throw std::runtime_error("DXIL resource metadata must have one resource table");
    auto* root = metadata->getOperand(0);
    if (!root) throw std::runtime_error("DXIL resource table is null");
    const auto& table = *root;
    if (table.getNumOperands() != 4) throw std::runtime_error("DXIL resource table must have four resource classes");
    for (unsigned resourceClass = 0; resourceClass < 4; ++resourceClass) {
        const auto& operand = table.getOperand(resourceClass);
        if (!operand) continue;
        const auto& list = Node(operand);
        for (unsigned index = 0; index < list.getNumOperands(); ++index) {
            auto binding = parse(Node(list.getOperand(index)), static_cast<ResourceClass>(resourceClass));
            if (std::ranges::any_of(bindings, [&](const auto& existing) { return existing.resourceClass == binding.resourceClass && existing.rangeId == binding.rangeId; })) throw std::runtime_error("DXIL resource range identifier is duplicated");
            bindings.push_back(binding);
        }
    }
}

const std::vector<BufferBinding>& ResourceLayout::GetBindings() const { return bindings; }

ResourceHandle ResourceLayout::ResolveHandle(const LLVMBC::CallInst& instruction) const {
    auto* function = instruction.getCalledFunction();
    if (!function) throw std::runtime_error("DXIL resource handle has no function declaration");
    const auto& name = function->getName();
    if (std::string_view(name.data(), name.size()) != "dx.op.createHandle" || instruction.getNumArgOperands() != 5) throw std::runtime_error("DXIL resource handle intrinsic has an invalid signature");
    auto* result = instruction.getType();
    if (!result || result->getTypeID() != LLVMBC::Type::TypeID::StructTyID || result->getStructNumElements() != 1 || result->getStructElementType(0)->getTypeID() != LLVMBC::Type::TypeID::PointerTyID) throw std::runtime_error("DXIL resource handle result is not an opaque handle");
    auto* pointer = result->getStructElementType(0);
    auto* element = pointer->getPointerElementType();
    if (pointer->getPointerAddressSpace() != 0 || !element->isIntegerTy() || element->getIntegerBitWidth() != 8) throw std::runtime_error("DXIL resource handle result has an incompatible pointer type");
    auto constant = [&](unsigned index, unsigned bits) {
        auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(instruction.getArgOperand(index));
        if (!value || !value->getType()->isIntegerTy() || value->getType()->getIntegerBitWidth() != bits) throw std::runtime_error("DXIL resource handle requires a correctly typed constant operand");
        return value->getUniqueInteger().getZExtValue();
    };
    if (constant(0, 32) != 57) throw std::runtime_error("DXIL resource handle opcode is invalid");
    auto category = constant(1, 8);
    if (category >= 4) throw std::runtime_error("DXIL resource handle class is invalid");
    auto range = constant(2, 32);
    auto nonUniform = constant(4, 1) != 0;
    auto found = std::ranges::find_if(bindings, [&](const auto& binding) { return static_cast<unsigned>(binding.resourceClass) == category && binding.rangeId == range; });
    if (found == bindings.end()) throw std::runtime_error("DXIL resource handle references an undeclared range");
    auto* index = instruction.getArgOperand(3);
    if (!index->getType()->isIntegerTy() || index->getType()->getIntegerBitWidth() != 32) throw std::runtime_error("DXIL resource handle index must be 32-bit");
    if (auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(index)) {
        auto address = value->getUniqueInteger().getZExtValue();
        if (address < found->firstRegister || (found->count != UINT32_MAX && address - found->firstRegister >= found->count)) throw std::runtime_error("DXIL resource handle index is outside its declared range");
    }
    return {&*found, index, nonUniform};
}

std::vector<BufferBinding> AnyBox::Shader::ReadDxilBufferBindings(std::span<const std::byte> code) {
    DxilModule module(code);
    return ResourceLayout(module.GetModule()).GetBindings();
}
