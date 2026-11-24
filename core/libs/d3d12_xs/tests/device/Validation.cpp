#include "Validation.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <iostream>
#include <memory>
#include <system_error>
#include <vector>

AnyBox::Graphics::Tests::NativeValidation::NativeValidation() : module(nullptr), debug(nullptr) {
    module = LoadLibraryExW(L"d3d12.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) Windows::ThrowLastError("LoadLibraryExW D3D12 validation");
    auto address = GetProcAddress(module, "D3D12GetDebugInterface");
    if (!address) { FreeLibrary(module); Windows::ThrowLastError("GetProcAddress D3D12GetDebugInterface"); }
    using GetDebug = HRESULT (WINAPI *)(REFIID, void**);
    GetDebug get;
    std::memcpy(&get, &address, sizeof(get));
    constexpr GUID interfaceId{0x344488B7, 0x6846, 0x474B, {0xB9, 0x89, 0xF0, 0x27, 0x44, 0x82, 0x45, 0xE0}};
    void* output = nullptr;
    auto result = get(interfaceId, &output);
    if (FAILED(result)) { FreeLibrary(module); throw std::system_error(static_cast<int>(result), std::system_category(), "D3D12 validation layer could not be enabled"); }
    if (!output) { FreeLibrary(module); throw std::runtime_error("D3D12 validation interface is null"); }
    debug = static_cast<ID3D12Debug*>(output);
    debug->EnableDebugLayer();
}

AnyBox::Graphics::Tests::NativeValidation::~NativeValidation() {
    debug->Release();
    FreeLibrary(module);
}

void AnyBox::Graphics::Tests::CheckValidation(Device* device) {
    constexpr GUID interfaceId{0x0742A90B, 0xC387, 0x483F, {0xB9, 0x46, 0x30, 0xA7, 0xE4, 0xE6, 0x14, 0x58}};
    void* output = nullptr;
    Require(device->GetHost()->QueryInterface(interfaceId, &output) == 0 && output, "D3D12 validation message queue is unavailable");
    auto release = [](ID3D12InfoQueue* value) { value->Release(); };
    std::unique_ptr<ID3D12InfoQueue, decltype(release)> queue(static_cast<ID3D12InfoQueue*>(output), release);
    Require(queue->GetNumMessagesDiscardedByMessageCountLimit() == 0, "D3D12 validation messages were discarded");
    bool valid = true;
    auto count = queue->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (std::uint64_t index = 0; index < count; ++index) {
        SIZE_T size = 0;
        Windows::CheckHResult(queue->GetMessage(index, nullptr, &size), "D3D12 validation message size query");
        Require(size >= sizeof(D3D12_MESSAGE), "D3D12 validation message size is invalid");
        std::vector<std::uint64_t> storage((size + 7) / 8);
        auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        Require(queue->GetMessage(index, message, &size) == 0, "D3D12 validation message query failed");
        if (message->Severity <= D3D12_MESSAGE_SEVERITY_WARNING) std::cerr << "D3D12 validation " << message->ID << ": " << message->pDescription << '\n';
        if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) valid = false;
    }
    Require(valid, "D3D12 validation reported an error");
}
