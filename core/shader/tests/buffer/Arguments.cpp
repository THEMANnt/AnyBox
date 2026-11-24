#include "TransferFixture.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"
#include "arguments/Bindings.hpp"
#include <Recompiler.hpp>

namespace {
void CheckTransfer(AnyBox::Vulkan::Context& context, bool structured) {
    BufferTests::TransferFixture fixture(16, structured);
    AnyBox::Shader::BufferArgumentPlan plan(fixture.GetBindings());
    AnyBox::Shader::Dxil::ArgumentBindings bindings(plan);
    auto program = fixture.Lower(bindings, plan.GetUserRegisterCount());
    ShaderRecompiler::ShaderComputeInputInfo input;
    input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> userData(plan.GetUserRegisterCount());
    userData[1] = 1;
    userData[2] = 64;
    auto output = plan.GetRanges()[1].firstUserRegister;
    userData[output + 1] = 2;
    userData[output + 2] = 120;
    std::vector<std::uint32_t> expected(30, UINT32_MAX);
    auto bytes = std::as_writable_bytes(std::span(expected));
    for (unsigned mask = 1; mask <= 15; ++mask) {
        for (unsigned component = 0; component < 4; ++component) {
            if ((mask & (1u << component)) == 0) continue;
            for (unsigned byte = 0; byte < 2; ++byte) {
                auto offset = component * 2 + byte;
                bytes[(mask - 1) * 8 + offset] = static_cast<std::byte>(((56 + offset) * 3 + 7) & 255);
            }
        }
    }
    BufferTests::RunGpu(context, shader, expected, {1, 1, 1}, {}, userData);
}

void CheckSelection(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Shader;
    using namespace ShaderRecompiler;
    BufferBinding binding{ResourceClass::ShaderResource, 0, 0, 4, 3, BufferKind::Raw};
    BufferArgumentPlan plan{std::span(&binding, 1)};
    Dxil::ArgumentBindings bindings(plan);
    IrProgram program;
    program.Resources().userDataCount = plan.GetUserRegisterCount();
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder() = {&block};
    program.Metadata().blockInfo.resize(1);
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    Fixtures::Reject<std::invalid_argument>([&] { bindings.ResolveBuffer(binding, builder.Constant(3), false, builder); });
    auto& lane = builder.Emit(IrOpcode::LaneId, IrType::U32, {});
    auto& index = builder.IAdd(lane, builder.Constant(3));
    auto selected = bindings.ResolveBuffer(binding, index, true, builder);
    auto& result = builder.IAdd(builder.CompositeExtract(selected.address, 0), builder.CompositeExtract(selected.byteLength, 0));
    auto& low = builder.ShiftLeftLogical(lane, builder.Constant(2));
    auto& high = builder.Constant(2);
    auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
    MemoryInfo memory;
    memory.kind = ResourceKind::Global;
    memory.addressIsFull = true;
    program.Resources().memoryInfo.push_back(memory);
    (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&address, &low, &high, &result, &builder.ConstantBool(true)}, MemoryFlags{0, 0});
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    ShaderComputeInputInfo input;
    input.threadsNum[0] = 6;
    input.threadsNum[1] = input.threadsNum[2] = 1;
    input.waveSize = input.hostSubgroupSize = 32;
    auto shader = CompileCompute(program, input, BufferTests::Target(), {0, 0, 0, 128});
    std::array<std::uint32_t, 12> userData{8, 1, 8, 0, 16, 1, 16, 0, 32, 1, 32, 0};
    std::array<std::uint32_t, 6> expected{0, 16, 32, 64, 0, 0};
    BufferTests::RunGpu(context, shader, expected, {1, 1, 1}, {}, userData);
}
}

void BufferTests::CheckBufferArgumentGpu(AnyBox::Vulkan::Context& context) {
    CheckTransfer(context, false);
    CheckTransfer(context, true);
    CheckSelection(context);
}
