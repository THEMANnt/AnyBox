#include "List.hpp"
#include "Compute.hpp"
#include <stdexcept>

using namespace AnyBox::Graphics;
using Container::RootParameterKind;

void CommandList::SetComputeRoot32BitConstants(CommandList* self, std::uint32_t parameter, std::uint32_t count, const std::uint32_t* values, std::uint32_t offset) {
    if (count != 0 && !values) throw std::invalid_argument("Compute root constants are null");
    if (!self->compute) throw std::logic_error("Compute root constants require a bound root signature");
    if (self->compute->GetArguments().GetKind(parameter) != RootParameterKind::Constants) throw std::invalid_argument("Compute root constants target a non-constant parameter");
    self->writeComputeArgument(parameter, offset, std::span(values, count));
}

void CommandList::SetComputeRoot32BitConstant(CommandList* self, std::uint32_t parameter, std::uint32_t value, std::uint32_t offset) {
    SetComputeRoot32BitConstants(self, parameter, 1, &value, offset);
}

void CommandList::SetComputeRootDescriptorTable(CommandList* self, std::uint32_t parameter, std::uint64_t address) {
    if (address > UINT32_MAX) throw std::invalid_argument("Xbox compute descriptor table address exceeds its 32-bit encoding");
    if (!self->compute) throw std::logic_error("Compute root table requires a bound root signature");
    if (self->compute->GetArguments().GetKind(parameter) != RootParameterKind::Table) throw std::invalid_argument("Compute root table targets a non-table parameter");
    auto word = static_cast<std::uint32_t>(address);
    self->writeComputeArgument(parameter, 0, std::span(&word, 1));
}

void CommandList::writeComputeDescriptor(std::uint32_t parameter, std::uint64_t address, std::uint32_t kind) {
    if (!compute) throw std::logic_error("Compute root descriptor requires a bound root signature");
    if (compute->GetArguments().GetKind(parameter) != static_cast<RootParameterKind>(kind)) throw std::invalid_argument("Compute root descriptor kind differs from the root signature");
    std::array words{static_cast<std::uint32_t>(address), static_cast<std::uint32_t>(address >> 32)};
    writeComputeArgument(parameter, 0, words);
}

void CommandList::SetComputeRootConstantBufferView(CommandList* self, std::uint32_t parameter, std::uint64_t address) { self->writeComputeDescriptor(parameter, address, static_cast<std::uint32_t>(RootParameterKind::ConstantBuffer)); }
void CommandList::SetComputeRootShaderResourceView(CommandList* self, std::uint32_t parameter, std::uint64_t address) { self->writeComputeDescriptor(parameter, address, static_cast<std::uint32_t>(RootParameterKind::ShaderResource)); }
void CommandList::SetComputeRootUnorderedAccessView(CommandList* self, std::uint32_t parameter, std::uint64_t address) { self->writeComputeDescriptor(parameter, address, static_cast<std::uint32_t>(RootParameterKind::UnorderedAccess)); }
