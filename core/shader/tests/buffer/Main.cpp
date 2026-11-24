#include "Fixture.hpp"
#include "Fixtures.hpp"
#include <anybox/ExceptionTest.hpp>
#include <bit>

namespace {
std::uint64_t Expected(unsigned offset, unsigned bits) {
    std::uint64_t value = 0;
    if (static_cast<std::uint64_t>(offset) + bits / 8 > 64) return 0;
    for (unsigned byte = 0; byte < bits / 8; ++byte) value |= static_cast<std::uint64_t>(((offset + byte) * 3 + 7) & 255) << (8 * byte);
    return value;
}

void CheckRaw(unsigned bits) {
    for (unsigned offset : {0u, 2u, 4u, 28u, 30u, 56u, 60u, 62u, 64u, UINT32_MAX}) {
        BufferTests::Fixture fixture(false);
        auto* load = fixture.Load(bits, offset, 0);
        auto& value = fixture.Extract(load, 0);
        auto& program = fixture.Finish();
        Fixtures::Require(BufferTests::Evaluate(program, value) == Expected(offset, bits));
    }
}

void CheckStructured(unsigned bits) {
    for (unsigned element : {0u, 1u, 2u, UINT32_MAX}) {
        for (unsigned offset : {0u, 8u, 24u, 28u, 30u, 32u}) {
            BufferTests::Fixture fixture(true);
            auto* load = fixture.Load(bits, element, offset);
            auto& value = fixture.Extract(load, 0);
            auto& program = fixture.Finish();
            auto expected = element >= 2 || offset + bits / 8 > 32 ? 0 : Expected(element * 32 + offset, bits);
            Fixtures::Require(BufferTests::Evaluate(program, value) == expected);
        }
    }
}

void CheckMasks(unsigned bits) {
    for (unsigned mask = 1; mask < 16; ++mask) {
        BufferTests::Fixture fixture(false);
        auto* load = fixture.Load(bits, 56, 0, mask);
        std::array<ShaderRecompiler::IrValue*, 4> outputs{};
        for (unsigned component = 0; component < 4; ++component) {
            if (mask & (1u << component)) outputs[component] = &fixture.Extract(load, component);
        }
        auto lowered = fixture.LowerFunction();
        Fixtures::Require(lowered.Resources().memoryInfo.size() == static_cast<std::size_t>(std::popcount(mask)));
        auto& program = fixture.Finish();
        for (unsigned component = 0; component < 4; ++component) {
            if (outputs[component]) Fixtures::Require(BufferTests::Evaluate(program, *outputs[component]) == Expected(56 + component * (bits / 8), bits));
        }
    }
}
}

int main() {
    return AnyBox::Tests::Run([] {
        BufferTests::CheckSpirv();
        for (unsigned bits : {16u, 32u, 64u}) {
            CheckRaw(bits);
            CheckStructured(bits);
            CheckMasks(bits);
        }
        {
            BufferTests::Fixture fixture(true);
            auto* load = fixture.Load(16, 1, 28, 3);
            auto& first = fixture.Extract(load, 0);
            auto& second = fixture.Extract(load, 1);
            Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Extract(load, 2); });
            Fixtures::Reject<std::runtime_error>([&] { (void)fixture.Extract(load, 4); });
            auto& program = fixture.Finish();
            Fixtures::Require(BufferTests::Evaluate(program, first) == Expected(60, 16));
            Fixtures::Require(BufferTests::Evaluate(program, second) == Expected(62, 16));
        }
        {
            BufferTests::Fixture coherent(true, 1);
            (void)coherent.Load(32, 0, 0);
            Fixtures::Require(coherent.Finish().Resources().memoryInfo.front().coherent);
        }
        BufferTests::Fixture invalid(false);
        Fixtures::Reject<std::runtime_error>([&] { (void)invalid.Load(32, 0, 0, 0); });
        Fixtures::Reject<std::runtime_error>([&] { (void)invalid.Load(32, 0, 0, 16); });
    });
}
