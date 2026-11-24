#include "TransferFixture.hpp"
#include "Target.hpp"

void BufferTests::CheckFloatTransfers(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    for (bool structured : {false, true}) {
        TransferFixture fixture(32, structured, false, true);
        auto program = fixture.Lower();
        ShaderComputeInputInfo inputInfo;
        inputInfo.threadsNum[0] = inputInfo.threadsNum[1] = inputInfo.threadsNum[2] = 1;
        inputInfo.waveSize = inputInfo.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, inputInfo, Target(), {0, 0, 0, 128});
        std::array<std::uint32_t, 16> input{};
        input[12] = 0x80000000;
        input[13] = 0x7FC12345;
        input[14] = 0x7F812345;
        input[15] = 1;
        std::vector<std::uint32_t> expected(60, UINT32_MAX);
        for (unsigned mask = 1; mask <= 15; ++mask) {
            for (unsigned component = 0; component < 4; ++component) {
                if (mask & (1u << component)) expected[(mask - 1) * 4 + component] = input[12 + component];
            }
        }
        RunGpu(context, shader, expected, {1, 1, 1}, std::as_bytes(std::span(input)));
    }
}
