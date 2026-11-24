#include "Compilation.hpp"
#include <anybox/shader/runtime/ComputePipeline.hpp>
#include <iostream>

void AnyBox::Shader::Tools::CheckArtifactGpu(const ComputeArtifact& artifact) {
    Vulkan::Context context(0);
    Runtime::ComputePipeline pipeline(context, artifact);
    std::cout << "gpu=" << context.GetProperties().deviceName << " pipeline=created\n";
}
