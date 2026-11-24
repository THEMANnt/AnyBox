#include "ResourceLayout.hpp"
#include "DxilMetadata.hpp"
#include <limits>

using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

BufferBinding ResourceLayout::parse(const LLVMBC::MDNode& node, ResourceClass resourceClass) {
    if (resourceClass == ResourceClass::Sampler) throw std::runtime_error("DXIL sampler resource metadata is not implemented");
    auto expected = resourceClass == ResourceClass::ShaderResource ? 9u : resourceClass == ResourceClass::UnorderedAccess ? 11u : 8u;
    if (node.getNumOperands() != expected) throw std::runtime_error("DXIL buffer resource metadata has an invalid field count");
    BufferBinding binding{resourceClass, Integer(node, 0), Integer(node, 3), Integer(node, 4), Integer(node, 5), BufferKind::Raw};
    if (binding.count == 0 || (binding.count != UINT32_MAX && static_cast<std::uint64_t>(binding.firstRegister) + binding.count > std::uint64_t{UINT32_MAX} + 1)) throw std::runtime_error("DXIL buffer binding range is invalid");
    if (resourceClass == ResourceClass::ConstantBuffer) {
        binding.kind = BufferKind::Constant;
        binding.byteSize = Integer(node, 6);
        if (binding.byteSize == 0) throw std::runtime_error("DXIL constant buffer has zero size");
        if (Operand(node, 7)) throw std::runtime_error("DXIL constant buffer extension properties are not implemented");
        return binding;
    }
    auto kind = Integer(node, 6);
    if (kind != 11 && kind != 12) throw std::runtime_error("DXIL resource is not a supported raw or structured buffer");
    binding.kind = kind == 11 ? BufferKind::Raw : BufferKind::Structured;
    if (resourceClass == ResourceClass::UnorderedAccess) {
        binding.globallyCoherent = Integer(node, 7, 1) != 0;
        binding.hasCounter = Integer(node, 8, 1) != 0;
        binding.rasterizerOrdered = Integer(node, 9, 1) != 0;
    } else if (Integer(node, 7) != 0) {
        throw std::runtime_error("DXIL buffer resource declares a multisample count");
    }
    const auto& extension = Operand(node, expected - 1);
    if (extension) {
        const auto& tags = Node(extension);
        if (tags.getNumOperands() != 2 || Integer(tags, 0) != 1 || binding.kind != BufferKind::Structured) throw std::runtime_error("DXIL buffer extension properties are not implemented");
        binding.stride = Integer(tags, 1);
    }
    if (binding.kind == BufferKind::Structured && (binding.stride == 0 || binding.stride > 2048 || binding.stride % 4 != 0)) throw std::runtime_error("DXIL structured buffer stride is invalid");
    if (binding.kind == BufferKind::Raw && binding.hasCounter) throw std::runtime_error("DXIL raw buffer cannot have a structured counter");
    return binding;
}
