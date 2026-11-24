#pragma once

#include <anybox/shader/Compute.hpp>

namespace LLVMBC { class Module; class Function; }

namespace AnyBox::Shader::Dxil {
struct ComputeEntry {
    ComputeInfo info;
    const LLVMBC::Function* function;
};

ComputeEntry ReadComputeEntry(const LLVMBC::Module& module);
}
