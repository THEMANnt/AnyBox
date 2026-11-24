#include "Commands.hpp"
#include <anybox/vulkan/Image.hpp>
#include <algorithm>
#include <utility>

namespace {
void CheckContents(AnyBox::Vulkan::Context& context, const AnyBox::Vulkan::ImageDescription& description) {
    using namespace AnyBox::Vulkan;
    using namespace AnyBox::Vulkan::Tests;
    Image image(context, description);
    Require(image.GetHandle() && image.GetAllocationSize() > 0, "Vulkan image allocation is empty");
    Image moved(std::move(image));
    Require(!image.GetHandle() && moved.GetHandle(), "Moved Vulkan image ownership was duplicated");
    std::vector<VkBufferImageCopy> regions;
    VkDeviceSize size = 0;
    for (std::uint32_t mip = 0; mip < description.mipLevels; ++mip) {
        VkBufferImageCopy region{};
        region.bufferOffset = size;
        region.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, mip, 0, description.arrayLayers};
        region.imageExtent = {std::max(1U, description.extent.width >> mip), std::max(1U, description.extent.height >> mip), std::max(1U, description.extent.depth >> mip)};
        size += static_cast<VkDeviceSize>(region.imageExtent.width) * region.imageExtent.height * region.imageExtent.depth * description.arrayLayers * 4;
        regions.push_back(region);
    }
    auto readback = Buffer::Allocate(context, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Commands commands(context);
    auto buffer = commands.Begin();
    const auto& functions = context.GetFunctions();
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = moved.GetHandle();
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, description.mipLevels, 0, description.arrayLayers};
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    VkClearColorValue color{};
    color.float32[1] = 1;
    color.float32[3] = 1;
    functions.clearColorImage(buffer, moved.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &barrier.subresourceRange);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    functions.copyImageToBuffer(buffer, moved.GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.GetHandle(), static_cast<std::uint32_t>(regions.size()), regions.data());
    VkMemoryBarrier host{};
    host.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    host.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    host.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &host, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
    auto bytes = readback.Map();
    for (std::size_t index = 0; index < bytes.size(); ++index) Require(bytes[index] == (index % 2 == 0 ? std::byte{0} : std::byte{255}), "Vulkan image mip or array contents did not survive GPU readback");
}
}

void AnyBox::Vulkan::Tests::CheckImages(Context& context) {
    ImageDescription description;
    description.format = VK_FORMAT_R8G8B8A8_UNORM;
    description.extent = {31, 19, 1};
    description.mipLevels = 5;
    description.arrayLayers = 3;
    description.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    CheckContents(context, description);
    description.type = VK_IMAGE_TYPE_3D;
    description.extent = {13, 9, 7};
    description.arrayLayers = 1;
    description.mipLevels = 4;
    CheckContents(context, description);
    description.type = VK_IMAGE_TYPE_1D;
    description.extent = {31, 1, 1};
    description.arrayLayers = 3;
    CheckContents(context, description);
    description.extent.width = 0;
    bool rejected = false;
    try { Image invalid(context, description); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected, "Vulkan image with invalid dimensions was accepted");
}
