#include "../resources/Fixture.hpp"
#include "lowering/Scalar.hpp"
#include "Target.hpp"
#include "Fixtures.hpp"

void BufferTests::CheckThreads(AnyBox::Vulkan::Context& gpu) {
    using namespace ShaderRecompiler;
    ResourceTests::Fixture source;
    auto& context = source.Context();
    IrProgram program;
    auto& block = program.CreateBlock();
    program.SetEntryBlock(block);
    program.BlockOrder().push_back(&block);
    program.Metadata().blockInfo.push_back({});
    IrBuilder builder(program);
    builder.SetInsertionPoint(block);
    AnyBox::Shader::Dxil::ScalarLowering scalar(program, builder);
    auto* word = LLVMBC::Type::getInt32Ty(context);
    auto call = [&](unsigned opcode, unsigned component) {
        std::array<const char*, 4> names{"dx.op.threadId.i32", "dx.op.groupId.i32", "dx.op.threadIdInGroup.i32", "dx.op.flattenedThreadIdInGroup.i32"};
        source.Module().add_value_name(opcode, names.at(opcode - 93));
        LLVMBC::Vector<LLVMBC::Type*> parameters{word};
        LLVMBC::Vector<LLVMBC::Value*> arguments{LLVMBC::ConstantInt::get(word, opcode)};
        if (opcode != 96) {
            parameters.push_back(word);
            arguments.push_back(LLVMBC::ConstantInt::get(word, component));
        }
        auto* signature = context.construct<LLVMBC::FunctionType>(context, word, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(signature, opcode, source.Module());
        return context.construct<LLVMBC::CallInst>(signature, function, std::move(arguments));
    };
    std::array<IrValue*, 10> coordinates{};
    for (unsigned operation = 0; operation < 3; ++operation) {
        for (unsigned component = 0; component < 3; ++component) coordinates[operation * 3 + component] = &scalar.Get(*call(93 + operation, component));
    }
    coordinates[9] = &scalar.Get(*call(96, 0));
    Fixtures::Reject<std::runtime_error>([&] { (void)scalar.Get(*call(93, 3)); });
    auto& row = builder.IAdd(*coordinates[1], builder.IMul(builder.Constant(4), *coordinates[2]));
    auto& index = builder.IAdd(*coordinates[0], builder.IMul(builder.Constant(8), row));
    auto& base = builder.IMul(index, builder.Constant(40));
    for (unsigned component = 0; component < coordinates.size(); ++component) {
        auto& low = builder.IAdd(base, builder.Constant(component * 4));
        auto& high = builder.Constant(2);
        auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &low, &high, coordinates[component], &builder.ConstantBool(true)}, flags);
    }
    (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    ShaderComputeInputInfo input;
    input.threadsNum[0] = 4;
    input.threadsNum[1] = input.threadsNum[2] = 2;
    input.waveSize = input.hostSubgroupSize = 32;
    auto shader = AnyBox::Shader::CompileCompute(program, input, Target(), {0, 0, 0, 128});
    std::vector<std::uint32_t> expected;
    for (unsigned z = 0; z < 4; ++z) {
        for (unsigned y = 0; y < 4; ++y) {
            for (unsigned x = 0; x < 8; ++x) {
                std::array values{x, y, z, x / 4, y / 2, z / 2, x % 4, y % 2, z % 2, x % 4 + 4 * (y % 2 + 2 * (z % 2))};
                expected.insert(expected.end(), values.begin(), values.end());
            }
        }
    }
    RunGpu(gpu, shader, expected, {2, 2, 2});
}
