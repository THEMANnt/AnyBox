#include "AtomicKernel.hpp"
#include <bit>

namespace BufferTests { void CheckGlobalAtomicInvalid(); }

namespace {
std::uint32_t Expected(std::uint32_t old, std::uint32_t value, bool maximum) {
    auto nan = [](std::uint32_t bits) { return (bits & 0x7FFFFFFF) > 0x7F800000; };
    auto signaling = [&](std::uint32_t bits) { return nan(bits) && (bits & 0x00400000) == 0; };
    if (signaling(value)) return value | 0x00400000;
    if (signaling(old)) return old | 0x00400000;
    if (nan(value) && nan(old)) return value;
    if (nan(value)) return old;
    if (nan(old)) return value;
    if (((old | value) & 0x7FFFFFFF) == 0) return maximum ? old & value : old | value;
    auto a = std::bit_cast<float>(old);
    auto b = std::bit_cast<float>(value);
    return (maximum ? b > a : b < a) ? value : old;
}

void CheckPairs(AnyBox::Vulkan::Context& context, bool maximum) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& builder = fixture.builder;
    auto& index = fixture.GlobalIndex();
    auto& inputOffset = builder.ShiftLeftLogical(builder.BitwiseAnd(index, builder.Constant(15)), builder.Constant(2));
    auto& input = builder.BitCastF32(fixture.Memory(IrOpcode::LoadAddressU32, inputOffset, 1));
    auto& offset = builder.ShiftLeftLogical(index, builder.Constant(2));
    auto& old = builder.BitCastU32(fixture.Atomic(maximum, offset, input));
    auto& returnedOffset = builder.IAdd(offset, builder.Constant(1024));
    fixture.Memory(IrOpcode::StoreAddressU32, returnedOffset, 2, &old);
    auto shader = fixture.Compile();
    std::array<std::uint32_t, 16> values{0x3F800000, 0xC0000000, 0, 0x80000000, 0x7F800000, 0xFF800000, 0x7FC12345, 0xFFC54321, 0x7F812345, 0xFF812345, 1, 0x80000001, 0x007FFFFF, 0x807FFFFF, 0x7F7FFFFF, 0xFF7FFFFF};
    std::vector<std::uint32_t> initial(512, 0xAAAAAAAA);
    std::vector<std::uint32_t> expected(512);
    for (unsigned sample = 0; sample < 256; ++sample) {
        auto oldBits = values[sample / 16];
        auto inputBits = values[sample % 16];
        initial[sample] = oldBits;
        expected[sample] = Expected(oldBits, inputBits, maximum);
        expected[sample + 256] = oldBits;
    }
    BufferTests::RunGpu(context, shader, expected, {4, 1, 1}, std::as_bytes(std::span(values)), {}, std::as_bytes(std::span(initial)));
}

void CheckConcurrent(AnyBox::Vulkan::Context& context, bool maximum) {
    using namespace ShaderRecompiler;
    BufferTests::AtomicKernel fixture;
    auto& builder = fixture.builder;
    auto before = maximum ? 0x3F800000U : 0x40000000U;
    auto after = maximum ? 0x40000000U : 0x3F800000U;
    auto& value = builder.BitCastF32(builder.Constant(after));
    auto& old = builder.BitCastU32(fixture.Atomic(maximum, builder.Constant(0), value));
    auto& first = builder.IEqual(old, builder.Constant(before));
    auto& count = builder.Select(first, builder.Constant(1), builder.Constant(0));
    fixture.Memory(IrOpcode::AddressAtomicIAdd32, builder.Constant(4), 2, &count);
    auto& valid = builder.LogicalOr(first, builder.IEqual(old, builder.Constant(after)));
    auto& invalid = builder.Select(valid, builder.Constant(0), builder.Constant(1));
    fixture.Memory(IrOpcode::AddressAtomicIAdd32, builder.Constant(8), 2, &invalid);
    auto shader = fixture.Compile();
    std::array initial{before, 0U, 0U};
    std::array expected{after, 1U, 0U};
    BufferTests::RunGpu(context, shader, expected, {16, 1, 1}, {}, {}, std::as_bytes(std::span(initial)));
}

void CheckUnaligned(AnyBox::Vulkan::Context& context, bool maximum) {
    BufferTests::AtomicKernel fixture;
    auto& builder = fixture.builder;
    fixture.Atomic(maximum, builder.Constant(1), builder.BitCastF32(builder.Constant(0x3F800000)));
    auto shader = fixture.Compile();
    std::array initial{0U};
    bool rejected = false;
    try {
        BufferTests::RunGpu(context, shader, initial, {1, 1, 1}, {}, {}, std::as_bytes(std::span(initial)));
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "DXIL BDA GPU test recorded fault 7") throw;
        rejected = true;
    }
    if (!rejected) throw std::runtime_error("Unaligned Xbox global atomic did not report a fault");
}
}

void BufferTests::CheckGlobalFloatAtomicGpu(AnyBox::Vulkan::Context& context) {
    CheckGlobalAtomicInvalid();
    for (bool maximum : {false, true}) {
        CheckPairs(context, maximum);
        CheckConcurrent(context, maximum);
        CheckUnaligned(context, maximum);
    }
}
