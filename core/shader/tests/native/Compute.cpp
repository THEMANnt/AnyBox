#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include "Recompiler.hpp"
#include <array>

void CheckSpirvValidation(std::span<const std::uint32_t> code);

void CheckNativeCompute() {
    using namespace Fixtures;
    using namespace AnyBox::Shader;
    auto data = MakeComputeProgram();
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    NativeFrontend frontend(data);
    auto target = BufferTests::Target();
    ShaderRecompiler::ShaderFloatMode mode{0xC0, true, true, false};
    auto result = LowerNativeCompute(frontend, state, mode, target);
    Require(result.input.threadsNum[0] == 4 && result.input.threadsNum[1] == 2 && result.input.threadsNum[2] == 2);
    Require(result.input.waveSize == 32 && result.input.hostSubgroupSize == 32);
    Require(result.input.workgroupRegister == 4 && result.input.threadIdsNum == 3);
    Require(result.input.groupId[0] && result.input.groupId[1] && result.input.groupId[2]);
    Require(result.input.ldsSizeDwords == 256 && !result.input.tgSizeEn);
    Require(result.program.Info().sharedMemoryBytes == 1024 && result.program.Resources().userDataCount == 4);
    auto compiled = CompileCompute(result.program, result.input, target, {0, 0, 0, 128});
    Require(compiled.code.size() >= 5 && compiled.code.front() == 0x07230203);
    CheckSpirvValidation(compiled.code);
    for (unsigned count = 0; count <= 16; ++count) {
        for (unsigned ids = 0; ids < 3; ++ids) {
            auto selected = state;
            selected.resourceControl = (state.resourceControl & ~0x183E) | (count << 1) | (ids << 11) | 0x400;
            auto lowered = LowerNativeCompute(frontend, selected, mode, target);
            Require(lowered.input.workgroupRegister == static_cast<int>(count));
            Require(lowered.input.threadIdsNum == static_cast<int>(ids + 1) && lowered.input.tgSizeEn);
        }
    }
    for (unsigned mask = 0; mask < 8; ++mask) {
        auto selected = state;
        selected.resourceControl = (state.resourceControl & ~0x380) | (mask << 7);
        auto lowered = LowerNativeCompute(frontend, selected, mode, target);
        for (unsigned axis = 0; axis < 3; ++axis) Require(lowered.input.groupId[axis] == ((mask & (1u << axis)) != 0));
    }
    for (unsigned bit : {0u, 6u, 13u, 14u, 24u, 30u, 31u}) {
        auto invalid = state;
        invalid.resourceControl |= 1u << bit;
        Reject<std::runtime_error>([&] { (void)LowerNativeCompute(frontend, invalid, mode, target); });
    }
    auto invalid = state;
    invalid.resourceControl |= 3u << 11;
    Reject<std::invalid_argument>([&] { (void)LowerNativeCompute(frontend, invalid, mode, target); });
    invalid = state;
    invalid.threads[0] = 0;
    Reject<std::invalid_argument>([&] { (void)LowerNativeCompute(frontend, invalid, mode, target); });
    target.subgroupSize = 16;
    Reject<std::invalid_argument>([&] { (void)LowerNativeCompute(frontend, state, mode, target); });
    auto legacy = MakeComputeProgram();
    Set(legacy, 0x168, 0xD5400000);
    Set(legacy, 0x16C, 0x00080200);
    Set(legacy, 0x170, 0xBF810000);
    NativeFrontend fused(legacy);
    Require(fused.GetDecoded().instructions.front().op == ShaderRecompiler::RdnaOpcode::VMadLegacyF32);
    auto fusedResult = LowerNativeCompute(fused, state, mode, BufferTests::Target());
    Require(fusedResult.program.Blocks().size() == fused.GetGraph().blocks.size() + 1);
}
