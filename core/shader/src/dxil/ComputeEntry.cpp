#include "ComputeEntry.hpp"
#include "DxilMetadata.hpp"
#include "DxilModule.hpp"
#include <function.hpp>
#include <bit>
#include <string_view>
#include <unordered_set>

using namespace AnyBox::Shader;
using namespace AnyBox::Shader::Dxil;

namespace {
const LLVMBC::MDNode& Named(const LLVMBC::Module& module, const char* name) {
    auto* named = module.getNamedMetadata(name);
    if (!named || named->getNumOperands() != 1 || !named->getOperand(0)) throw std::runtime_error(std::string("DXIL compute metadata must have one ") + name + " node");
    return *named->getOperand(0);
}

std::string Text(const LLVMBC::MDNode& node, unsigned index) {
    auto* metadata = LLVMBC::dyn_cast<LLVMBC::MDString>(Operand(node, index));
    if (!metadata) throw std::runtime_error("DXIL compute metadata string is missing");
    const auto& value = metadata->getString();
    if (value.empty() || std::string_view(value.data(), value.size()).find('\0') != std::string_view::npos) throw std::runtime_error("DXIL compute metadata string is invalid");
    return {value.data(), value.size()};
}

void Wave(std::uint32_t value) {
    if (!std::has_single_bit(value) || value < 4 || value > 128) throw std::runtime_error("DXIL wave size is invalid");
}
}

ComputeEntry AnyBox::Shader::Dxil::ReadComputeEntry(const LLVMBC::Module& module) {
    const auto& model = Named(module, "dx.shaderModel");
    if (model.getNumOperands() != 3 || Text(model, 0) != "cs" || Integer(model, 1) != 6) throw std::runtime_error("DXIL shader model is not a supported compute stage");
    const auto& entry = Named(module, "dx.entryPoints");
    if (entry.getNumOperands() != 5) throw std::runtime_error("DXIL compute entry point has an invalid field count");
    auto* functionMetadata = LLVMBC::dyn_cast<LLVMBC::ConstantAsMetadata>(Operand(entry, 0));
    auto* function = functionMetadata ? LLVMBC::dyn_cast<LLVMBC::Function>(functionMetadata->getValue()) : nullptr;
    if (!function || function->empty()) throw std::runtime_error("DXIL compute entry point does not reference a defined function");
    ComputeEntry result{{Text(entry, 1), {}, 0, Integer(model, 2)}, function};
    const auto& properties = Node(Operand(entry, 4));
    if (properties.getNumOperands() % 2 != 0) throw std::runtime_error("DXIL compute entry properties are not tag-value pairs");
    std::unordered_set<std::uint32_t> tags;
    for (unsigned index = 0; index < properties.getNumOperands(); index += 2) {
        auto tag = Integer(properties, index);
        if (!tags.insert(tag).second) throw std::runtime_error("DXIL compute entry property is duplicated");
        if (tag == 0) {
            auto* metadata = LLVMBC::dyn_cast<LLVMBC::ConstantAsMetadata>(Operand(properties, index + 1));
            auto* flags = metadata ? LLVMBC::dyn_cast<LLVMBC::ConstantInt>(metadata->getValue()) : nullptr;
            if (!flags || !flags->getType()->isIntegerTy() || flags->getType()->getIntegerBitWidth() != 64) throw std::runtime_error("DXIL shader flags must be a 64-bit constant");
            result.info.flags = flags->getUniqueInteger().getZExtValue();
        } else if (tag == 4) {
            const auto& threads = Node(Operand(properties, index + 1));
            if (threads.getNumOperands() != 3) throw std::runtime_error("DXIL compute workgroup must have three dimensions");
            std::uint64_t product = 1;
            for (unsigned axis = 0; axis < 3; ++axis) {
                auto size = Integer(threads, axis);
                if (size == 0 || size > 1024 || (axis == 2 && size > 64)) throw std::runtime_error("DXIL compute workgroup dimension is invalid");
                product *= size;
                result.info.threads[axis] = size;
            }
            if (product > 1024) throw std::runtime_error("DXIL compute workgroup exceeds 1024 threads");
        } else if (tag == 11 || tag == 23) {
            const auto& wave = Node(Operand(properties, index + 1));
            if (wave.getNumOperands() != (tag == 11 ? 1u : 3u)) throw std::runtime_error("DXIL wave size property has an invalid field count");
            result.info.minimumWaveSize = Integer(wave, 0);
            result.info.maximumWaveSize = tag == 11 ? result.info.minimumWaveSize : Integer(wave, 1);
            result.info.preferredWaveSize = tag == 11 ? result.info.minimumWaveSize : Integer(wave, 2);
            Wave(result.info.minimumWaveSize);
            Wave(result.info.maximumWaveSize);
            if (result.info.preferredWaveSize != 0) Wave(result.info.preferredWaveSize);
            if (result.info.minimumWaveSize > result.info.maximumWaveSize || (result.info.preferredWaveSize != 0 && (result.info.preferredWaveSize < result.info.minimumWaveSize || result.info.preferredWaveSize > result.info.maximumWaveSize))) throw std::runtime_error("DXIL wave size range is inconsistent");
        } else {
            throw std::runtime_error("DXIL compute entry property is not implemented: " + std::to_string(tag));
        }
    }
    if (!tags.contains(4)) throw std::runtime_error("DXIL compute entry point has no workgroup size");
    if (tags.contains(11) && tags.contains(23)) throw std::runtime_error("DXIL compute entry point has conflicting wave size properties");
    return result;
}

ComputeInfo AnyBox::Shader::ReadDxilComputeInfo(std::span<const std::byte> code) {
    DxilModule module(code);
    return ReadComputeEntry(module.GetModule()).info;
}
