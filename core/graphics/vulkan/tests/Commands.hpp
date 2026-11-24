#pragma once

#include "Tests.hpp"

namespace AnyBox::Vulkan::Tests {
class Commands {
public:
    explicit Commands(AnyBox::Vulkan::Context& context) : context(context) {
        const auto& functions = context.GetFunctions();
        VkCommandPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        info.queueFamilyIndex = context.GetQueueFamily();
        AnyBox::Vulkan::Check(functions.createCommandPool(context.GetDevice(), &info, nullptr, &pool), "Create Vulkan transfer test pool");
    }
    ~Commands() {
        const auto& functions = context.GetFunctions();
        if (pending) {
            auto result = functions.queueWaitIdle(context.GetQueue(0));
            if (result != VK_SUCCESS && result != VK_ERROR_DEVICE_LOST) std::terminate();
        }
        if (fence) functions.destroyFence(context.GetDevice(), fence, nullptr);
        functions.destroyCommandPool(context.GetDevice(), pool, nullptr);
    }
    VkCommandBuffer Begin() {
        VkCommandBufferAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        info.commandPool = pool;
        info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        info.commandBufferCount = 1;
        VkCommandBuffer buffer = VK_NULL_HANDLE;
        AnyBox::Vulkan::Check(context.GetFunctions().allocateCommandBuffers(context.GetDevice(), &info, &buffer), "Allocate Vulkan transfer test commands");
        VkCommandBufferBeginInfo begin{};
        begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        AnyBox::Vulkan::Check(context.GetFunctions().beginCommandBuffer(buffer, &begin), "Begin Vulkan transfer test commands");
        return buffer;
    }
    void Submit(VkCommandBuffer buffer) {
        const auto& functions = context.GetFunctions();
        AnyBox::Vulkan::Check(functions.endCommandBuffer(buffer), "End Vulkan transfer test commands");
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        AnyBox::Vulkan::Check(functions.createFence(context.GetDevice(), &fenceInfo, nullptr, &fence), "Create Vulkan transfer test fence");
        VkSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        info.commandBufferCount = 1;
        info.pCommandBuffers = &buffer;
        context.Submit(0, info, fence);
        pending = true;
        AnyBox::Vulkan::Check(functions.waitForFences(context.GetDevice(), 1, &fence, VK_TRUE, 5000000000ull), "Wait Vulkan transfer test completion");
        pending = false;
    }

private:
    AnyBox::Vulkan::Context& context;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool pending = false;
};
}
