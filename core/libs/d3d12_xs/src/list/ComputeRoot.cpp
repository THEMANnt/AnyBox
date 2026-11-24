#include "List.hpp"
#include "Compute.hpp"
#include <memory>
#include <stdexcept>

void AnyBox::Graphics::CommandList::SetComputeRootSignature(CommandList* self, void* signature) {
    if (self->IsClosed()) throw std::logic_error("Compute root signature binding requires a recording command list");
    if (self->guest.type != D3D12_COMMAND_LIST_TYPE_DIRECT && self->guest.type != D3D12_COMMAND_LIST_TYPE_COMPUTE) throw std::runtime_error("Compute root binding is not implemented for this command list type");
    auto* root = signature ? RootSignature::From(signature, self->device) : nullptr;
    self->flushGuestCommands();
    if (root && self->compute && self->compute->GetRoot() == root) return;
    std::unique_ptr<ComputeState> next;
    if (root) next = std::make_unique<ComputeState>(root);
    delete self->compute;
    self->compute = next.release();
    self->updateRootHeaders();
}

void AnyBox::Graphics::CommandList::writeComputeArgument(std::uint32_t parameter, std::uint32_t offset, std::span<const std::uint32_t> words) {
    if (IsClosed()) throw std::logic_error("Compute root updates require a recording command list");
    flushGuestCommands();
    if (!compute) throw std::logic_error("Compute root update requires a bound root signature");
    compute->GetArguments().Write(parameter, offset, words);
}
