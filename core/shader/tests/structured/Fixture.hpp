#pragma once

#include "ComputeCompiler.hpp"
#include "../buffer/Target.hpp"
#include <IntermediateRepresentation/IrBuilder.hpp>
#include <array>

namespace StructuredTests {
class Fixture {
public:
    using IrValue = ShaderRecompiler::IrValue;
    using IrBlock = ShaderRecompiler::IrBlock;
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder;
    IrBlock* entry;
    IrValue* index;
    IrValue* outputOffset;

    Fixture() : builder(program) {
        using namespace ShaderRecompiler;
        entry = &Block();
        program.SetEntryBlock(*entry);
        program.SetWaveSize(32);
        Enter(*entry);
        index = &builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(StageInputKind::GlobalInvocationId)), &builder.Constant(0)});
        outputOffset = &builder.ShiftLeftLogical(*index, builder.Constant(2));
    }
    IrBlock& Block() {
        auto& block = program.CreateBlock();
        program.BlockOrder().push_back(&block);
        ShaderRecompiler::BlockInfo info;
        info.id = block.Id();
        program.Metadata().blockInfo.push_back(info);
        return block;
    }
    void Enter(IrBlock& block) { builder.SetInsertionPoint(block); }
    void Branch(IrBlock& block, IrBlock& target) {
        Enter(block);
        builder.Branch(target);
        auto& term = program.Metadata().blockInfo.at(block.Id()).terminator;
        term.kind = ShaderRecompiler::TerminatorKind::Branch;
        term.trueBlock = target.Id();
    }
    void Conditional(IrBlock& block, IrValue& condition, IrBlock& first, IrBlock& second) {
        Enter(block);
        builder.BranchConditional(condition, first, second);
        auto& info = program.Metadata().blockInfo.at(block.Id());
        info.condition = &condition;
        info.terminator.kind = ShaderRecompiler::TerminatorKind::ConditionalBranch;
        info.terminator.condition = ShaderRecompiler::BranchCondition::ScalarInstruction;
        info.terminator.trueBlock = first.Id();
        info.terminator.falseBlock = second.Id();
    }
    void Return(IrBlock& block, IrValue& value) {
        using namespace ShaderRecompiler;
        Enter(block);
        auto& high = builder.Constant(2);
        auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {outputOffset, &high});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&address, outputOffset, &high, &value, &builder.ConstantBool(true)}, flags);
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
    }
    void Run(AnyBox::Vulkan::Context& context, std::span<const std::uint32_t> expected) {
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 32;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::Dxil::CompileCompute(program, info, BufferTests::Target(), {0, 0, 0, 128});
        BufferTests::RunGpu(context, shader, expected, {1, 1, 1});
    }
};
void CheckBranches(AnyBox::Vulkan::Context& context);
void CheckLoop(AnyBox::Vulkan::Context& context);
void CheckTyped(AnyBox::Vulkan::Context& context);
void CheckExits(AnyBox::Vulkan::Context& context);
void CheckClones(AnyBox::Vulkan::Context& context);
void CheckFloatLoop(AnyBox::Vulkan::Context& context);
}
