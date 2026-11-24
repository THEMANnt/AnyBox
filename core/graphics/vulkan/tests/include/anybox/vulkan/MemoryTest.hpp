#pragma once

#include <anybox/vulkan/Buffer.hpp>

namespace AnyBox::Vulkan::Tests {
void Copy(Context& context, const Buffer& source, const Buffer& destination, VkDeviceSize size);
void CheckCompute(Context& context, Buffer& imported, Buffer& scratch, bool repeated);
void CheckAddress(Context& context, VkDeviceAddress address, std::span<const std::byte> expected);
}
