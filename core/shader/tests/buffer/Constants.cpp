#include "ConstantFixture.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"

void BufferTests::CheckConstants(AnyBox::Vulkan::Context& context) {
    CheckRootConstants(context);
    {
        ConstantFixture fixture;
        auto program = fixture.Lower();
        ShaderRecompiler::ShaderComputeInputInfo input;
        input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
        input.waveSize = input.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128});
        std::array<std::uint32_t, 60> expected{};
        auto bytes = std::as_writable_bytes(std::span(expected));
        for (unsigned kind = 0; kind < 3; ++kind) {
            for (unsigned offset = 0; offset < 64; ++offset) bytes[kind * 80 + offset] = static_cast<std::byte>((offset * 3 + 7) & 255);
        }
        RunGpu(context, shader, expected, {1, 1, 1});
    }

    ConstantFixture invalid(true);
    Fixtures::Reject<std::runtime_error>([&] { (void)invalid.Lower(); });
}
