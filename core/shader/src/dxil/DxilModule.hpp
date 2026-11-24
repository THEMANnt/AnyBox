#pragma once

#include "Session.hpp"
#include <context.hpp>
#include <module.hpp>
#include <cstddef>
#include <span>

#undef llvm

namespace AnyBox::Shader {
class DxilModule {
public:
    explicit DxilModule(std::span<const std::byte> code);
    LLVMBC::Module& GetModule() const;
    DxilModule(const DxilModule&) = delete;
    DxilModule& operator=(const DxilModule&) = delete;

private:
    DxilSession session;
    LLVMBC::LLVMContext context;
    LLVMBC::Module* module = nullptr;
};
}
