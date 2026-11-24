#include <Recompiler.hpp>
#include <anybox/ExceptionTest.hpp>
#include <anybox/vulkan/ComputePipeline.hpp>
#include "Commands.hpp"
#include <array>
#include <stdexcept>
#include <string_view>

void CheckThreadStorage();

namespace {
void CheckCompute(AnyBox::Vulkan::Context& context, std::uint32_t waveSize) {
    constexpr std::array<std::uint32_t, 1> code{0xBF810000};
    constexpr std::array<std::uint32_t, 2> capabilities{1, 61};
    constexpr std::array<std::string_view, 1> extensions{"SPV_KHR_storage_buffer_storage_class"};
    ShaderRecompiler::RecompileRequest request{};
    request.shader = {ShaderRecompiler::ShaderStage::Compute, 0x20000, code, 0, {}};
    request.context.waveSize = waveSize;
    request.context.compute = ShaderRecompiler::ShaderComputeStageInfo{{waveSize, 1, 1}, 0, {false, false, false}, false, 1, {}};
    request.target.vulkanVersion = 0x00402000;
    request.target.spirvVersion = 0x00010500;
    request.target.subgroupSize = waveSize;
    request.target.bdaAbiVersion = 1;
    request.target.supportedCapabilities = capabilities;
    request.target.supportedExtensions = extensions;
    request.target.maxWorkgroupSize = {1024, 1024, 64};
    request.target.maxWorkgroupInvocations = 1024;
    request.target.maxWorkgroupSharedMemoryBytes = 49152;
    request.layout = {0, 0, 0, 128};
    request.useCache = false;
    const auto result = ShaderRecompiler::Recompile(request);
    if (result.spirv.size() < 5 || result.spirv[0] != 0x07230203) throw std::runtime_error("RDNA compute recompilation returned no SPIR-V module");
    AnyBox::Vulkan::ComputePipeline pipeline(context, std::span(result.spirv.data(), result.spirv.size()), 128);
    AnyBox::Vulkan::Tests::Commands commands(context);
    auto buffer = commands.Begin();
    const std::array<std::uint32_t, 32> constants{};
    pipeline.Dispatch(buffer, std::as_bytes(std::span(constants)), {1, 1, 1});
    commands.Submit(buffer);
}
}

int main() {
    return AnyBox::Tests::Run([] {
        CheckThreadStorage();
        AnyBox::Vulkan::Context context(0);
        CheckCompute(context, 32);
        CheckCompute(context, 64);
    });
}
