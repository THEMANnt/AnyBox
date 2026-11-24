#include "Commands.hpp"
#include <anybox/vulkan/ComputePipeline.hpp>
#include <algorithm>
#include <cstring>

namespace {
constexpr auto Source = R"(#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require
layout(local_size_x = 64) in;
layout(buffer_reference, std430, buffer_reference_align = 4) buffer Data { uint values[]; };
layout(push_constant) uniform Parameters { Data source; Data destination; uint count; uint salt; } args;
void main() {
    uint index = gl_GlobalInvocationID.x;
    if (index < args.count) args.destination.values[index] = args.source.values[index] ^ args.salt;
}
)";

struct Parameters {
    VkDeviceAddress source;
    VkDeviceAddress destination;
    std::uint32_t count;
    std::uint32_t salt;
};
static_assert(sizeof(Parameters) == 24);

void Dispatch(AnyBox::Vulkan::Context& context, const AnyBox::Vulkan::ComputePipeline& pipeline, const Parameters& parameters) {
    using namespace AnyBox::Vulkan::Tests;
    Commands commands(context);
    auto buffer = commands.Begin();
    const auto& functions = context.GetFunctions();
    VkMemoryBarrier before{};
    before.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    before.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    before.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1, &before, 0, nullptr, 0, nullptr);
    pipeline.Dispatch(buffer, std::as_bytes(std::span(&parameters, 1)), {(parameters.count + 63) / 64, 1, 1});
    VkMemoryBarrier after{};
    after.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    after.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    after.dstAccessMask = VK_ACCESS_HOST_READ_BIT;
    functions.pipelineBarrier(buffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, 0, 1, &after, 0, nullptr, 0, nullptr);
    commands.Submit(buffer);
}

void Verify(std::span<const std::byte> observed, std::span<const std::byte> source, std::uint32_t salt) {
    for (std::size_t index = 0; index < observed.size(); index += sizeof(std::uint32_t)) {
        std::uint32_t input = 0;
        std::uint32_t output = 0;
        std::memcpy(&input, source.data() + index, sizeof(input));
        std::memcpy(&output, observed.data() + index, sizeof(output));
        AnyBox::Vulkan::Tests::Require(output == (input ^ salt), "Vulkan shader did not access the actual mapped XMem pages through device addresses");
    }
}
}

void AnyBox::Vulkan::Tests::CheckCompute(Context& context, Buffer& imported, Buffer& scratch, bool repeated) {
    auto code = CompileCompute(Source);
    ComputePipeline pipeline(context, code, sizeof(Parameters));
    auto memory = imported.Map();
    std::vector<std::byte> expected(memory.begin(), memory.end());
    auto count = static_cast<std::uint32_t>(memory.size() / sizeof(std::uint32_t));
    constexpr std::uint32_t readSalt = 0x78142ca3;
    Dispatch(context, pipeline, {imported.GetAddress(), scratch.GetAddress(), count, readSalt});
    auto observed = scratch.Map();
    Verify(observed, expected, readSalt);

    constexpr std::uint32_t writeSalt = 0xa913ce72;
    expected.assign(observed.begin(), observed.end());
    if (repeated) count = PhysicalPageSize / sizeof(std::uint32_t);
    Dispatch(context, pipeline, {scratch.GetAddress(), imported.GetAddress(), count, writeSalt});
    Verify(memory, expected, writeSalt);
}

void AnyBox::Vulkan::Tests::CheckAddress(Context& context, VkDeviceAddress address, std::span<const std::byte> expected) {
    if (expected.empty() || expected.size() % 4 != 0 || expected.size() / 4 > UINT32_MAX) throw std::invalid_argument("Vulkan address test size is invalid");
    constexpr auto usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    auto scratch = Buffer::Allocate(context, expected.size(), usage, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    std::ranges::fill(scratch.Map(), std::byte{0xA7});
    auto code = CompileCompute(Source);
    ComputePipeline pipeline(context, code, sizeof(Parameters));
    Dispatch(context, pipeline, {address, scratch.GetAddress(), static_cast<std::uint32_t>(expected.size() / 4), 0});
    Require(std::ranges::equal(scratch.Map(), expected), "Vulkan shader did not read the expected data through its supplied device address");
}
