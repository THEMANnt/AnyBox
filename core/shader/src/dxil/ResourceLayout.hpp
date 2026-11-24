#pragma once

#include <anybox/shader/Resources.hpp>
#include <instruction.hpp>
#include <metadata.hpp>

namespace LLVMBC { class Module; }

namespace AnyBox::Shader::Dxil {
struct ResourceHandle {
    const BufferBinding* binding;
    const LLVMBC::Value* index;
    bool nonUniform;
};

class ResourceLayout {
public:
    explicit ResourceLayout(const LLVMBC::Module& module);
    const std::vector<BufferBinding>& GetBindings() const;
    ResourceHandle ResolveHandle(const LLVMBC::CallInst& instruction) const;

private:
    static BufferBinding parse(const LLVMBC::MDNode& node, ResourceClass resourceClass);
    std::vector<BufferBinding> bindings;
};
}
