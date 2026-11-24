#include "Target.hpp"
#include "Fixtures.hpp"
#include "../native/MaskLoopFixture.hpp"
#include "../../src/native/FirstLane.hpp"
#include <Recompiler.hpp>
#include <vector>

namespace {
void AddOutput(NativeMaskTests::Fixture& fixture) {
    using namespace ShaderRecompiler;
    auto& program = fixture.program;
    IrBuilder builder(program);
    builder.SetInsertionPoint(*program.BlockOrder().at(1));
    auto& ballot = builder.Emit(IrOpcode::Ballot, IrType::Vec4U32, {fixture.mask});
    auto& any = builder.BitwiseOr(builder.CompositeExtract(ballot, 0), builder.CompositeExtract(ballot, 1));
    auto& condition = builder.INotEqual(any, builder.Constant(0));
    program.Metadata().blockInfo.at(1).condition = &condition;
    (void)builder.Emit(IrOpcode::Reference, IrType::Void, {&condition});
    builder.SetInsertionPoint(*program.BlockOrder().at(2));
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& low = builder.ShiftLeftLogical(lane, builder.Constant(2));
    auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &builder.Constant(2)});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&address, &low, &builder.Constant(2), fixture.read, fixture.chosen}, MemoryFlags{0, 0});
    ValidateProgram(program, true);
}
}

void BufferTests::CheckBallotLoopGpu(AnyBox::Vulkan::Context& context) {
    for (unsigned variant = 0; variant < 4; ++variant) {
        std::uint32_t waveSize = (variant & 1) != 0 ? 64 : 32;
        bool bitWords = (variant & 2) != 0;
        for (bool optimize : {false, true}) {
            NativeMaskTests::Fixture fixture(waveSize, NativeMaskTests::Mode::Narrow, bitWords);
            AddOutput(fixture);
            if (optimize) Fixtures::Require(AnyBox::Shader::SimplifyFirstLaneGuards(fixture.program) == 1);
            ShaderRecompiler::ShaderComputeInputInfo input;
            input.threadsNum[0] = waveSize;
            input.threadsNum[1] = input.threadsNum[2] = 1;
            input.waveSize = waveSize;
            input.hostSubgroupSize = 32;
            auto shader = AnyBox::Shader::CompileCompute(fixture.program, input, Target(), {0, 0, 0, 128});
            std::vector<std::uint32_t> expected(waveSize, UINT32_MAX);
            for (unsigned lane = 0; lane < waveSize; ++lane) {
                bool active = bitWords ? ((0xFF00FFFFFFFFFE00ULL >> lane) & 1) != 0 : lane > 8;
                if (active) expected[lane] = (lane + 1) << 5;
            }
            RunGpu(context, shader, expected, {1, 1, 1});
        }
    }
}
