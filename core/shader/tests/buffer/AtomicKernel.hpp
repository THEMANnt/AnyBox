#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Scalar.hpp"
#include "Target.hpp"

namespace BufferTests {
struct AtomicKernel {
    using IrValue = ShaderRecompiler::IrValue;
    using IrOpcode = ShaderRecompiler::IrOpcode;
    using IrType = ShaderRecompiler::IrType;
    ResourceTests::Fixture source;
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder;
    AnyBox::Shader::Dxil::ScalarLowering scalar;
    unsigned nextId = 1;

    AtomicKernel() : builder(program), scalar(program, builder) {
        program.SetWaveSize(32);
        auto& block = program.CreateBlock();
        program.SetEntryBlock(block);
        program.BlockOrder() = {&block};
        program.Metadata().blockInfo.resize(1);
        builder.SetInsertionPoint(block);
    }
    IrValue& GlobalIndex() {
        return builder.Emit(IrOpcode::GetBuiltin, IrType::U32, {&builder.Constant(static_cast<unsigned>(ShaderRecompiler::StageInputKind::GlobalInvocationId)), &builder.Constant(0)});
    }
    IrValue& Memory(IrOpcode opcode, IrValue& offset, unsigned high, IrValue* data = nullptr) {
        using namespace ShaderRecompiler;
        auto& upper = builder.Constant(high);
        auto& address = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&offset, &upper});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        auto type = IrOpcodeType(opcode);
        if (data) return builder.Emit(opcode, type, {&address, &offset, &upper, data, &builder.ConstantBool(true)}, flags);
        return builder.Emit(opcode, type, {&address, &offset, &upper, &builder.ConstantBool(true)}, flags);
    }
    IrValue& Atomic(bool maximum, IrValue& offset, IrValue& value) {
        auto& context = source.Context();
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* wide = LLVMBC::Type::getInt64Ty(context);
        auto* floating = LLVMBC::Type::getFloatTy(context);
        auto* address = context.construct<LLVMBC::Argument>(wide, 0, false);
        auto* input = context.construct<LLVMBC::Argument>(floating, 1, false);
        scalar.Bind(*address, builder.ConstructU64(offset, builder.Constant(2)));
        scalar.Bind(*input, value);
        auto id = nextId++;
        source.Module().add_value_name(id, maximum ? "xbox.global_atomic_fmax" : "xbox.global_atomic_fmin");
        auto* type = context.construct<LLVMBC::FunctionType>(context, floating, LLVMBC::Vector<LLVMBC::Type*>{word, wide, floating});
        auto* function = context.construct<LLVMBC::Function>(type, id, source.Module());
        auto* call = context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, maximum ? 369 : 368), address, input});
        return scalar.Get(*call);
    }
    AnyBox::Shader::ComputeCompilation Compile() {
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
        ShaderRecompiler::ShaderComputeInputInfo info;
        info.threadsNum[0] = 64;
        info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        return AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
    }
};
}
