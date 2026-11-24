#include "Commands.hpp"

void AnyBox::Vulkan::Tests::Copy(Context& context, const Buffer& source, const Buffer& destination, VkDeviceSize size) {
    if (size == 0 || size > source.GetSize() || size > destination.GetSize()) throw std::invalid_argument("Vulkan transfer test range is invalid");
    Commands commands(context);
    auto buffer = commands.Begin();
    const auto& functions = context.GetFunctions();
    VkMemoryBarrier before{};
    before.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    before.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
    before.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &before, 0, nullptr, 0, nullptr);
    VkBufferCopy region{0, 0, size};
    functions.copyBuffer(buffer, source.GetHandle(), destination.GetHandle(), 1, &region);
    VkMemoryBarrier after{};
    after.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    after.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &after, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
}
