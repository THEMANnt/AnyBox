#include "TransferFixture.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"

void BufferTests::CheckTransfers(AnyBox::Vulkan::Context& context) {
    using namespace ShaderRecompiler;
    for (unsigned bits : {16u, 32u, 64u}) {
        for (bool structured : {false, true}) {
            TransferFixture fixture(bits, structured);
            auto program = fixture.Lower();
            ShaderComputeInputInfo input;
            input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
            input.waveSize = input.hostSubgroupSize = 32;
            auto shader = AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128});
            auto payload = bits / 2;
            std::vector<std::uint32_t> expected(15 * payload / 4, UINT32_MAX);
            auto bytes = std::as_writable_bytes(std::span(expected));
            for (unsigned mask = 1; mask <= 15; ++mask) {
                for (unsigned component = 0; component < 4; ++component) {
                    if ((mask & (1u << component)) == 0) continue;
                    for (unsigned byte = 0; byte < bits / 8; ++byte) {
                        auto offset = component * (bits / 8) + byte;
                        bytes[(mask - 1) * payload + offset] = static_cast<std::byte>(((64 - payload + offset) * 3 + 7) & 255);
                    }
                }
            }
            RunGpu(context, shader, expected, {1, 1, 1});
        }
        TransferFixture invalid(bits, false, true);
        Fixtures::Reject<std::runtime_error>([&] { (void)invalid.Lower(); });
    }
}
