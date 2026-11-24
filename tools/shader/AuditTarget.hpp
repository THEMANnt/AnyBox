#pragma once

#include <SpirvBackend/SpirvEmitter.hpp>
#include <spirv/unified1/spirv.hpp>
#include <array>

namespace AnyBox::Shader::Tools {
inline ShaderRecompiler::SpirvTargetOptions AuditTarget() {
    static constexpr std::array<std::uint32_t, 5> capabilities{spv::CapabilityShader, spv::CapabilityInt64, spv::CapabilityGroupNonUniform, spv::CapabilityPhysicalStorageBufferAddresses, spv::CapabilityStorageBuffer8BitAccess};
    static constexpr std::array<std::string_view, 3> extensions{"SPV_KHR_storage_buffer_storage_class", "SPV_KHR_physical_storage_buffer", "SPV_KHR_8bit_storage"};
    ShaderRecompiler::SpirvTargetOptions target{};
    target.vulkanVersion = 0x00402000;
    target.spirvVersion = 0x00010500;
    target.subgroupSize = 32;
    target.bdaAbiVersion = 1;
    target.supportedCapabilities = capabilities;
    target.supportedExtensions = extensions;
    return target;
}
}
