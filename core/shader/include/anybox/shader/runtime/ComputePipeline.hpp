#pragma once

#include <anybox/shader/Artifact.hpp>
#include <anybox/vulkan/ComputePipeline.hpp>

namespace AnyBox::Shader::Runtime {
class ComputePipeline {
public:
    ComputePipeline(Vulkan::Context& context, const ComputeArtifact& artifact);
    ~ComputePipeline();
    ComputePipeline(const ComputePipeline&) = delete;
    ComputePipeline& operator=(const ComputePipeline&) = delete;
    const Vulkan::ComputePipeline& GetPipeline() const noexcept { return *pipeline; }
    VkDescriptorSetLayout GetDescriptorLayout() const noexcept { return descriptorLayout; }

private:
    Vulkan::Context& context;
    VkDescriptorSetLayout descriptorLayout = VK_NULL_HANDLE;
    std::unique_ptr<Vulkan::ComputePipeline> pipeline;
};
}
