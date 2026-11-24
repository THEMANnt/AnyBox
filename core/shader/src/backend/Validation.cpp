#include "Validation.hpp"
#include "Environment.hpp"
#include <spirv-tools/libspirv.h>
#include <memory>
#include <stdexcept>
#include <string>

spv_target_env AnyBox::Shader::GetSpirvEnvironment(std::uint32_t version) {
    switch (version & ~0xFFFu) {
        case 0x400000: return SPV_ENV_VULKAN_1_0;
        case 0x401000: return SPV_ENV_VULKAN_1_1;
        case 0x402000: return SPV_ENV_VULKAN_1_2;
        case 0x403000: return SPV_ENV_VULKAN_1_3;
        case 0x404000: return SPV_ENV_VULKAN_1_4;
        default: throw std::invalid_argument("Shader validation target Vulkan version is not supported");
    }
}

void AnyBox::Shader::ValidateSpirv(std::span<const std::uint32_t> code, std::uint32_t vulkanVersion) {
    std::unique_ptr<spv_context_t, decltype(&spvContextDestroy)> context(spvContextCreate(GetSpirvEnvironment(vulkanVersion)), &spvContextDestroy);
    if (!context) throw std::bad_alloc();
    spv_diagnostic raw = nullptr;
    auto status = spvValidateBinary(context.get(), code.data(), code.size(), &raw);
    std::unique_ptr<spv_diagnostic_t, decltype(&spvDiagnosticDestroy)> diagnostic(raw, &spvDiagnosticDestroy);
    if (status == SPV_SUCCESS) return;
    std::string message = "SPIR-V validation failed";
    if (diagnostic && diagnostic->error) message += " at word " + std::to_string(diagnostic->position.index) + ": " + diagnostic->error;
    throw std::runtime_error(message);
}
