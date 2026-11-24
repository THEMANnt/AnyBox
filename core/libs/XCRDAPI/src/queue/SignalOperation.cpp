#include "SignalOperation.hpp"

#include <anybox/platform/Windows.hpp>
#include <d3d12.h>

#include <stdexcept>
#include <utility>

AnyBox::Storage::SignalOperation::SignalOperation(ID3D12Fence* input, std::uint64_t value) : original(input), value(value) {
    if (!input) throw std::invalid_argument("DirectStorage fence is null");
    constexpr GUID interfaceId{0x0A753DCF, 0xC4D8, 0x4B91, {0xAD, 0xF6, 0xBE, 0x5A, 0x60, 0xD9, 0x5A, 0x76}};
    void* output = nullptr;
    auto result = input->QueryInterface(interfaceId, &output);
    if (FAILED(result)) throw std::invalid_argument("DirectStorage fence does not expose ID3D12Fence");
    fence = static_cast<ID3D12Fence*>(output);
    if (!fence) throw std::runtime_error("DirectStorage fence returned a null interface");
}

AnyBox::Storage::SignalOperation::SignalOperation(SignalOperation&& other) noexcept : fence(std::exchange(other.fence, nullptr)), original(other.original), value(other.value) {}

AnyBox::Storage::SignalOperation::~SignalOperation() {
    if (fence) fence->Release();
}

std::int32_t AnyBox::Storage::SignalOperation::Complete() { return fence->Signal(value); }
StorageSignalFailure AnyBox::Storage::SignalOperation::GetFailure() const { return {original, value}; }
