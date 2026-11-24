#include <anybox/shader/Inspection.hpp>
#include "DxilModule.hpp"

std::string AnyBox::Shader::InspectDxil(std::span<const std::byte> code) {
    DxilModule module(code);
    LLVMBC::String assembly;
    if (!LLVMBC::disassemble(module.GetModule(), assembly)) throw std::runtime_error("DXIL disassembly failed");
    if (assembly.empty()) throw std::runtime_error("DXIL disassembler returned no assembly");
    return {assembly.data(), assembly.size()};
}
