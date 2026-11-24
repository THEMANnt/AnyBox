#include "Heap.hpp"

void AnyBox::Graphics::DescriptorHeap::GetDescription(DescriptorHeap* self, D3D12_DESCRIPTOR_HEAP_DESC* description) {
    if (!description) throw std::invalid_argument("Descriptor heap description output is null");
    *description = {self->type, self->count, self->flags, 0};
}

std::uint64_t AnyBox::Graphics::DescriptorHeap::getStart() const noexcept {
    auto base = allocation ? reinterpret_cast<std::uintptr_t>(allocation) : address;
    auto offset = (base << 32) - (address << 32);
    return (address + (type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 16 : 0)) | offset;
}

std::uint64_t AnyBox::Graphics::DescriptorHeap::GetCpuStart(DescriptorHeap* self) noexcept { return self->getStart(); }
std::uint64_t AnyBox::Graphics::DescriptorHeap::GetGpuStart(DescriptorHeap* self) noexcept { return self->getStart(); }
