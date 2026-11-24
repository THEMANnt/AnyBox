#pragma once

#include "../root/Signature.hpp"
#include <anybox/graphics/commands/RootState.hpp>

namespace AnyBox::Graphics {
class ComputeState {
public:
    explicit ComputeState(RootSignature* root) : root(root), arguments(root->GetLayout()) { RootSignature::AddRef(root); }
    ~ComputeState() { RootSignature::Release(root); }
    ComputeState(const ComputeState&) = delete;
    ComputeState& operator=(const ComputeState&) = delete;
    RootSignature* GetRoot() const noexcept { return root; }
    Commands::RootState& GetArguments() noexcept { return arguments; }
    const Commands::RootState& GetArguments() const noexcept { return arguments; }

private:
    RootSignature* root;
    Commands::RootState arguments;
};
}
