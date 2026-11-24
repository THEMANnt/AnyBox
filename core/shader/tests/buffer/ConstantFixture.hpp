#pragma once

#include "TransferFixture.hpp"

namespace BufferTests {
class ConstantBindings : public AnyBox::Shader::Dxil::IResourceBindings {
public:
    AnyBox::Shader::Dxil::BufferAddress ResolveBuffer(const AnyBox::Shader::BufferBinding& binding, ShaderRecompiler::IrValue& index, bool nonUniform, ShaderRecompiler::IrBuilder& builder) override {
        if (!index.HasImmediate() || index.ImmediateU32() != 4 || nonUniform) throw std::runtime_error("Unexpected constant buffer test selection");
        bool input = binding.resourceClass == AnyBox::Shader::ResourceClass::ConstantBuffer;
        return {builder.ConstantU64(input ? 0x100000000 : 0x200000000), builder.ConstantU64(input ? 64 : 240)};
    }
};

class ConstantFixture {
public:
    explicit ConstantFixture(bool invalid = false, bool root = false, unsigned byteSize = 52, bool dynamic = false) {
        auto& context = source.Context();
        auto& module = source.Module();
        auto count = root ? 1u : 3u;
        auto* table = source.Node({source.Null(), source.Node({source.Buffer(1, 0, 4, count)}), source.Node({source.Buffer(2, 32, 4, count, byteSize)}), source.Null()});
        module.add_named_metadata("dx.resources", context.construct<LLVMBC::NamedMDNode>(&module, "dx.resources", LLVMBC::Vector<LLVMBC::MDNode*>{table}));
        layout.emplace(module);
        block = context.construct<LLVMBC::BasicBlock>(context);
        auto* input = source.Handle(invalid ? 1 : 2, 4);
        auto* output = source.Handle(1, 4);
        block->add_instruction(input);
        block->add_instruction(output);
        auto* thread = dynamic ? call("dx.op.threadId.i32", LLVMBC::Type::getInt32Ty(context), {integer(32, 93), integer(32, 0)}) : nullptr;
        unsigned offset = 0;
        for (unsigned kind = 0; kind < 3; ++kind) {
            unsigned bits = kind == 1 ? 64 : 32;
            auto* element = kind == 2 ? LLVMBC::Type::getFloatTy(context) : LLVMBC::Type::getIntTy(context, bits);
            LLVMBC::Vector<LLVMBC::Type*> members(128 / bits, element);
            auto* result = LLVMBC::StructType::get(context, std::move(members), "constant");
            std::string suffix = kind == 2 ? "f32" : "i" + std::to_string(bits);
            for (unsigned row = 0; row < 5; ++row) {
                LLVMBC::Value* selected = integer(32, row);
                if (thread) {
                    auto* sum = context.construct<LLVMBC::BinaryOperator>(thread, selected, LLVMBC::Instruction::BinaryOps::Add);
                    block->add_instruction(sum);
                    selected = sum;
                }
                auto* loaded = call("dx.op.cbufferLoadLegacy." + suffix, result, {integer(32, 59), input, selected});
                LLVMBC::Vector<LLVMBC::Value*> arguments{integer(32, 140), output, integer(32, offset), LLVMBC::UndefValue::get(LLVMBC::Type::getInt32Ty(context))};
                for (unsigned component = 0; component < 4; ++component) {
                    if (component >= 128 / bits) {
                        arguments.push_back(LLVMBC::UndefValue::get(element));
                        continue;
                    }
                    auto* value = context.construct<LLVMBC::ExtractValueInst>(element, loaded, LLVMBC::Vector<unsigned>{component});
                    block->add_instruction(value);
                    arguments.push_back(value);
                }
                arguments.push_back(integer(8, bits == 64 ? 3 : 15));
                arguments.push_back(integer(32, bits / 8));
                call("dx.op.rawBufferStore." + suffix, LLVMBC::Type::getVoidTy(context), std::move(arguments));
                offset += 16;
            }
        }
        block->add_instruction(context.construct<LLVMBC::ReturnInst>(nullptr));
    }
    std::span<const AnyBox::Shader::BufferBinding> GetBindings() const { return layout->GetBindings(); }
    ShaderRecompiler::IrProgram Lower(AnyBox::Shader::Dxil::IResourceBindings* selected = nullptr, unsigned userDataCount = 0) {
        auto& context = source.Context();
        auto* type = context.construct<LLVMBC::FunctionType>(context, LLVMBC::Type::getVoidTy(context), LLVMBC::Vector<LLVMBC::Type*>{});
        auto* function = context.construct<LLVMBC::Function>(type, 1024, source.Module());
        function->set_basic_blocks({block});
        ShaderRecompiler::IrProgram output;
        output.Resources().userDataCount = userDataCount;
        AnyBox::Shader::Dxil::FunctionLowering(output, *layout, selected ? *selected : bindings).Run(*function);
        return output;
    }

private:
    LLVMBC::ConstantInt* integer(unsigned bits, unsigned value) {
        return LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(source.Context(), bits), value);
    }
    LLVMBC::CallInst* call(const std::string& name, LLVMBC::Type* result, LLVMBC::Vector<LLVMBC::Value*> arguments) {
        auto& context = source.Context();
        auto& module = source.Module();
        unsigned id = nextId++;
        module.add_value_name(id, LLVMBC::String(name.begin(), name.end()));
        LLVMBC::Vector<LLVMBC::Type*> parameters;
        for (auto* argument : arguments) parameters.push_back(argument->getType());
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, std::move(parameters));
        auto* function = context.construct<LLVMBC::Function>(type, id, module);
        auto* instruction = context.construct<LLVMBC::CallInst>(type, function, std::move(arguments));
        block->add_instruction(instruction);
        return instruction;
    }
    ResourceTests::Fixture source;
    ConstantBindings bindings;
    std::optional<AnyBox::Shader::Dxil::ResourceLayout> layout;
    LLVMBC::BasicBlock* block;
    unsigned nextId = 1;
};

void CheckConstants(AnyBox::Vulkan::Context& context);
void CheckRootConstants(AnyBox::Vulkan::Context& context);
}
