#pragma once

#include <metadata.hpp>
#include <cast.hpp>
#include <type.hpp>
#include <stdexcept>

namespace AnyBox::Shader::Dxil {
inline const LLVMBC::MDNode& Node(const LLVMBC::MDOperand& operand) {
    auto* node = LLVMBC::dyn_cast<LLVMBC::MDNode>(operand);
    if (!node) throw std::runtime_error("DXIL metadata operand is not a node");
    return *node;
}

inline const LLVMBC::MDOperand& Operand(const LLVMBC::MDNode& node, unsigned index) {
    if (index >= node.getNumOperands()) throw std::runtime_error("DXIL metadata node is missing an operand");
    return node.getOperand(index);
}

inline std::uint32_t Integer(const LLVMBC::MDNode& node, unsigned index, unsigned bits = 32) {
    auto* metadata = LLVMBC::dyn_cast<LLVMBC::ConstantAsMetadata>(Operand(node, index));
    if (!metadata) throw std::runtime_error("DXIL metadata operand is not a constant");
    auto* value = LLVMBC::dyn_cast<LLVMBC::ConstantInt>(metadata->getValue());
    if (!value || !value->getType()->isIntegerTy() || value->getType()->getIntegerBitWidth() != bits) throw std::runtime_error("DXIL metadata constant has an invalid integer type");
    return static_cast<std::uint32_t>(value->getUniqueInteger().getZExtValue());
}
}
