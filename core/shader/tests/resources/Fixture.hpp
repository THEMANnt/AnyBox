#pragma once

#include "ResourceLayout.hpp"
#include "Session.hpp"
#include <context.hpp>
#include <function.hpp>
#include <module.hpp>
#include <type.hpp>

#undef llvm

namespace ResourceTests {
class Fixture {
public:
    Fixture() : module(context) {}
    LLVMBC::MDOperand* Integer(std::uint64_t value, unsigned bits = 32) {
        return context.construct<LLVMBC::ConstantAsMetadata>(&module, LLVMBC::ConstantInt::get(LLVMBC::Type::getIntTy(context, bits), value));
    }
    LLVMBC::MDNode* Node(std::initializer_list<LLVMBC::MDOperand*> operands) {
        return context.construct<LLVMBC::MDNode>(&module, LLVMBC::Vector<LLVMBC::MDOperand*>(operands));
    }
    LLVMBC::MDOperand* Null() { return context.construct<LLVMBC::MDOperand>(&module); }
    LLVMBC::MDNode* Buffer(unsigned category, unsigned stride = 32, unsigned firstRegister = 4, unsigned count = 3, unsigned byteSize = 52) {
        auto* symbol = context.construct<LLVMBC::ConstantAsMetadata>(&module, LLVMBC::UndefValue::get(LLVMBC::PointerType::get(LLVMBC::Type::getInt32Ty(context), 0)));
        auto* name = context.construct<LLVMBC::MDString>(&module, "buffer");
        if (category == 2) return Node({Integer(7), symbol, name, Integer(2), Integer(firstRegister), Integer(count), Integer(byteSize), Null()});
        auto* extra = stride == 0 ? Null() : static_cast<LLVMBC::MDOperand*>(Node({Integer(1), Integer(stride)}));
        if (category == 0) return Node({Integer(7), symbol, name, Integer(2), Integer(firstRegister), Integer(count), Integer(stride == 0 ? 11 : 12), Integer(0), extra});
        return Node({Integer(7), symbol, name, Integer(2), Integer(firstRegister), Integer(count), Integer(stride == 0 ? 11 : 12), Integer(1, 1), Integer(0, 1), Integer(0, 1), extra});
    }
    void Set(unsigned category, LLVMBC::MDNode* buffer) {
        LLVMBC::Vector<LLVMBC::MDOperand*> classes(4);
        for (auto& entry : classes) entry = Null();
        classes.at(category) = Node({buffer});
        auto* table = context.construct<LLVMBC::MDNode>(&module, std::move(classes));
        module.add_named_metadata("dx.resources", context.construct<LLVMBC::NamedMDNode>(&module, "dx.resources", LLVMBC::Vector<LLVMBC::MDNode*>{table}));
    }
    LLVMBC::CallInst* Handle(unsigned category, unsigned index, bool dynamic = false) {
        module.add_value_name(0, "dx.op.createHandle");
        auto* word = LLVMBC::Type::getInt32Ty(context);
        auto* byte = LLVMBC::Type::getInt8Ty(context);
        auto* boolean = LLVMBC::Type::getInt1Ty(context);
        auto* result = LLVMBC::StructType::get(context, {LLVMBC::PointerType::get(byte, 0)}, "handle");
        auto* type = context.construct<LLVMBC::FunctionType>(context, result, LLVMBC::Vector<LLVMBC::Type*>{word, byte, word, word, boolean});
        auto* function = context.construct<LLVMBC::Function>(type, 0, module);
        auto* selected = dynamic ? static_cast<LLVMBC::Value*>(context.construct<LLVMBC::Argument>(word, 0, false)) : LLVMBC::ConstantInt::get(word, index);
        return context.construct<LLVMBC::CallInst>(type, function, LLVMBC::Vector<LLVMBC::Value*>{LLVMBC::ConstantInt::get(word, 57), LLVMBC::ConstantInt::get(byte, category), LLVMBC::ConstantInt::get(word, 7), selected, LLVMBC::ConstantInt::get(boolean, dynamic)});
    }
    LLVMBC::Module& Module() { return module; }
    LLVMBC::LLVMContext& Context() { return context; }

private:
    AnyBox::Shader::DxilSession session;
    LLVMBC::LLVMContext context;
    LLVMBC::Module module;
};
void CheckComputeEntries();
}
