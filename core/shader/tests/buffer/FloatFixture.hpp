#pragma once

#include "../resources/Fixture.hpp"
#include "lowering/Scalar.hpp"
#include "Target.hpp"

namespace BufferTests {
class FloatFixture {
public:
    FloatFixture() : builder(program), scalar(program, builder) {
        using namespace ShaderRecompiler;
        program.SetWaveSize(32);
        auto& block = program.CreateBlock();
        program.SetEntryBlock(block);
        program.BlockOrder().push_back(&block);
        program.Metadata().blockInfo.push_back({});
        builder.SetInsertionPoint(block);
        for (unsigned group = 0; group < 4; ++group) {
            auto& low = builder.Constant(group * 16);
            auto& high = builder.Constant(1);
            auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
            MemoryInfo memory;
            memory.kind = ResourceKind::Global;
            memory.addressIsFull = true;
            memory.dataDwords = memory.componentCount = 4;
            MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
            program.Resources().memoryInfo.push_back(memory);
            auto& loaded = builder.Emit(IrOpcode::LoadAddressU32x4, IrType::Vec4U32, {&resource, &low, &high, &builder.ConstantBool(true)}, flags);
            for (unsigned index = 0; index < 4; ++index) {
                auto* argument = source.Context().construct<LLVMBC::Argument>(LLVMBC::Type::getFloatTy(source.Context()), group * 4 + index, false);
                scalar.Bind(*argument, builder.BitCastF32(builder.CompositeExtract(loaded, index)));
                inputArguments[group * 4 + index] = argument;
            }
        }
    }
    LLVMBC::Value* Argument(unsigned index) { return inputArguments.at(index); }
    LLVMBC::ConstantInt* Integer(unsigned bits, std::uint64_t value) { return LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(source.Context(), bits), value); }
    LLVMBC::Type* FloatType() { return LLVMBC::Type::getFloatTy(source.Context()); }
    LLVMBC::ConstantFP* FloatConstant(std::uint32_t bits) { return LLVMBC::ConstantFP::get(FloatType(), bits); }
    LLVMBC::Type* IntegerType(unsigned bits) { return LLVMBC::Type::getIntTy(source.Context(), bits); }
    template<class TInstruction, class... TArguments>
    TInstruction* Make(TArguments&&... arguments) { return source.Context().construct<TInstruction>(std::forward<TArguments>(arguments)...); }
    LLVMBC::CallInst* Call(const std::string& name, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        auto& module = source.Module();
        auto id = nextId++;
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(type, id, module);
        return context.construct<LLVMBC::CallInst>(type, function, std::move(arguments));
    }
    void Expect(const LLVMBC::Value& value, std::uint32_t reference) {
        using namespace ShaderRecompiler;
        auto& result = scalar.Get(value);
        auto& bits = result.Type() == IrType::F32 ? builder.BitCastU32(result) : result;
        output(bits, reference);
    }
    void Predicates(LLVMBC::Value* left, LLVMBC::Value* right, std::uint32_t reference) {
        using namespace ShaderRecompiler;
        IrValue* mask = &builder.Constant(0);
        for (unsigned predicate = 0; predicate < 16; ++predicate) {
            auto* comparison = Make<LLVMBC::FCmpInst>(static_cast<LLVMBC::Instruction::Predicate>(predicate), left, right);
            auto& bit = builder.Select(scalar.Get(*comparison), builder.Constant(1u << predicate), builder.Constant(0));
            mask = &builder.BitwiseOr(*mask, bit);
        }
        output(*mask, reference);
    }
    void Reject(const LLVMBC::Value& value) {
        bool caught = false;
        try { (void)scalar.Get(value); } catch (const std::runtime_error&) { caught = true; }
        if (!caught) throw std::runtime_error("Unsupported floating instruction was accepted");
    }
    void Run(AnyBox::Vulkan::Context& context, std::span<const std::uint32_t> input) {
        using namespace ShaderRecompiler;
        (void)builder.Emit(IrOpcode::Return, IrType::Void, {});
        ShaderComputeInputInfo info;
        info.threadsNum[0] = info.threadsNum[1] = info.threadsNum[2] = 1;
        info.waveSize = info.hostSubgroupSize = 32;
        auto shader = AnyBox::Shader::CompileCompute(program, info, Target(), {0, 0, 0, 128});
        RunGpu(context, shader, expected, {1, 1, 1}, std::as_bytes(input));
    }

private:
    void output(ShaderRecompiler::IrValue& value, std::uint32_t reference) {
        using namespace ShaderRecompiler;
        auto& low = builder.Constant(static_cast<std::uint32_t>(expected.size() * 4));
        auto& high = builder.Constant(2);
        auto& resource = builder.Emit(IrOpcode::GetAddressResource, IrType::AddressResource, {&low, &high});
        MemoryInfo memory;
        memory.kind = ResourceKind::Global;
        memory.addressIsFull = true;
        MemoryFlags flags{static_cast<std::uint32_t>(program.Resources().memoryInfo.size()), 0};
        program.Resources().memoryInfo.push_back(memory);
        (void)builder.Emit(IrOpcode::StoreAddressU32, IrType::Void, {&resource, &low, &high, &value, &builder.ConstantBool(true)}, flags);
        expected.push_back(reference);
    }
    ResourceTests::Fixture source;
    ShaderRecompiler::IrProgram program;
    ShaderRecompiler::IrBuilder builder;
    AnyBox::Shader::Dxil::ScalarLowering scalar;
    std::array<LLVMBC::Value*, 16> inputArguments{};
    std::vector<std::uint32_t> expected;
    unsigned nextId = 1;
};

void CheckFloats(AnyBox::Vulkan::Context& context);
void CheckBitFields(AnyBox::Vulkan::Context& context);
}
