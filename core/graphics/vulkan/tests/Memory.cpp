#include "Tests.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>

void AnyBox::Vulkan::Tests::CheckLayout(Context& context, Storage& storage, std::span<const std::uint64_t> frames) {
    storage.Map(frames);
    auto memory = storage.GetMemory();
    Lease lease(AnyBoxAcquireMappedPhysicalPages(memory.data(), frames.size()));
    constexpr auto usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto imported = Buffer::Import(context, memory, memory.size(), usage);
    Require(imported.Map().data() == memory.data(), "Vulkan imported buffer lost its original CPU address");
    auto address = imported.GetAddress();
    Require(address != 0, "Vulkan imported buffer has no device address");
    for (std::size_t index = 0; index < memory.size(); ++index) memory[index] = static_cast<std::byte>((index * 71) ^ (index >> 9));
    std::vector<std::byte> expected(memory.begin(), memory.end());
    auto copied = Buffer::Allocate(context, memory.size(), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    Copy(context, imported, copied, memory.size());
    auto observed = copied.Map();
    Require(std::ranges::equal(observed, expected), "Vulkan GPU did not read the actual mapped XMem pages");
    for (std::size_t index = 0; index < observed.size(); ++index) observed[index] = static_cast<std::byte>((index * 37) ^ (index >> 8));
    bool repeated = frames[0] == frames[1] && frames[1] == frames[2];
    Copy(context, copied, imported, repeated ? PhysicalPageSize : memory.size());
    Require(std::ranges::equal(memory, observed), "Vulkan GPU writes did not reach the mapped XMem pages");
    CheckCompute(context, imported, copied, repeated);
    std::cout << "Verified transfer and SPIR-V device-address reads/writes over " << memory.size() << " bytes across " << frames.size() << " mapped pages; GPU address 0x" << std::hex << address << std::dec << '\n';
}
