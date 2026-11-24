#pragma once

#include <anybox/vulkan/Buffer.hpp>
#include <IntermediateRepresentation/IrProgram.hpp>

namespace BufferTests {
class Descriptors {
public:
    Descriptors(AnyBox::Vulkan::Context& context, AnyBox::Vulkan::Buffer& table, AnyBox::Vulkan::Buffer& faults) : context(context) {
        try { initialize(table, faults); } catch (...) { release(); throw; }
    }
    ~Descriptors() { release(); }
    Descriptors(const Descriptors&) = delete;
    Descriptors& operator=(const Descriptors&) = delete;
    VkDescriptorSetLayout GetLayout() const { return layout; }
    VkDescriptorSet GetSet() const { return set; }

private:
    void initialize(AnyBox::Vulkan::Buffer& table, AnyBox::Vulkan::Buffer& faults) {
        using namespace ShaderRecompiler;
        using AnyBox::Vulkan::Check;
        const auto& functions = context.GetFunctions();
        std::array<VkDescriptorSetLayoutBinding, 2> bindings{};
        bindings[0].binding = NativeBinding(IrShaderStage::Compute, DescriptorBindingKind::BdaPagetable);
        bindings[1].binding = NativeBinding(IrShaderStage::Compute, DescriptorBindingKind::FaultBuffer);
        for (auto& binding : bindings) {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        VkDescriptorSetLayoutCreateInfo description{};
        description.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        description.bindingCount = static_cast<std::uint32_t>(bindings.size());
        description.pBindings = bindings.data();
        Check(functions.createDescriptorSetLayout(context.GetDevice(), &description, nullptr, &layout), "Create BDA test descriptor layout");
        VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2};
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = 1;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &size;
        Check(functions.createDescriptorPool(context.GetDevice(), &poolInfo, nullptr, &pool), "Create BDA test descriptor pool");
        VkDescriptorSetAllocateInfo allocation{};
        allocation.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocation.descriptorPool = pool;
        allocation.descriptorSetCount = 1;
        allocation.pSetLayouts = &layout;
        Check(functions.allocateDescriptorSets(context.GetDevice(), &allocation, &set), "Allocate BDA test descriptor set");
        std::array<VkDescriptorBufferInfo, 2> buffers{{{table.GetHandle(), 0, table.GetSize()}, {faults.GetHandle(), 0, faults.GetSize()}}};
        std::array<VkWriteDescriptorSet, 2> writes{};
        for (std::size_t index = 0; index < writes.size(); ++index) {
            writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[index].dstSet = set;
            writes[index].dstBinding = bindings[index].binding;
            writes[index].descriptorCount = 1;
            writes[index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            writes[index].pBufferInfo = &buffers[index];
        }
        functions.updateDescriptorSets(context.GetDevice(), static_cast<std::uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
    void release() noexcept {
        const auto& functions = context.GetFunctions();
        if (pool) functions.destroyDescriptorPool(context.GetDevice(), pool, nullptr);
        if (layout) functions.destroyDescriptorSetLayout(context.GetDevice(), layout, nullptr);
    }
    AnyBox::Vulkan::Context& context;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
};
}
