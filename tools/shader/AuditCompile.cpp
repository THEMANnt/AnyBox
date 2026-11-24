#include "Audit.hpp"
#include "Compilation.hpp"

std::size_t AnyBox::Shader::Tools::AuditCompute(std::span<const std::byte> code) {
    auto result = CompileDxil(code);
    if (!result.buffers.empty()) throw std::runtime_error("DXIL compute SPIR-V generated with reflected root arguments; application dispatch execution is not verified");
    return result.code.size();
}
