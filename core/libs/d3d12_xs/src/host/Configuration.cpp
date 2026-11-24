#include "Library.hpp"
#include <d3d12.h>
#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string_view>

namespace {
std::filesystem::path ModulePath(HMODULE module) {
    std::array<wchar_t, 32768> buffer;
    auto count = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (count == 0) AnyBox::Windows::ThrowLastError("GetModuleFileNameW D3D12 runtime");
    if (count >= buffer.size()) throw std::length_error("D3D12 runtime module path is truncated");
    return std::filesystem::path(std::wstring_view(buffer.data(), count));
}

bool ApplicationConfigured() {
    auto module = GetModuleHandleW(nullptr);
    if (!module) AnyBox::Windows::ThrowLastError("GetModuleHandleW executable");
    auto version = GetProcAddress(module, "D3D12SDKVersion");
    auto path = GetProcAddress(module, "D3D12SDKPath");
    if (!version && !path) return false;
    if (!version || !path) throw std::runtime_error("Application D3D12 SDK configuration is incomplete");
    const void* address;
    static_assert(sizeof(address) == sizeof(version));
    std::memcpy(&address, &version, sizeof(address));
    std::uint32_t value;
    std::memcpy(&value, address, sizeof(value));
    if (value != ANYBOX_AGILITY_SDK_VERSION) throw std::runtime_error("Application D3D12 SDK version does not match AnyBox");
    return true;
}
}

void AnyBox::Graphics::ConfigureHostRuntime(const HostLibrary& library) {
    if (ApplicationConfigured()) return;
    static int anchor;
    HMODULE owner = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<const wchar_t*>(&anchor), &owner)) Windows::ThrowLastError("GetModuleHandleExW graphics library");
    auto directory = ModulePath(owner).parent_path() / L"D3D12";
    auto relative = directory.lexically_relative(ModulePath(nullptr).parent_path());
    if (relative.empty() || relative.is_absolute()) throw std::runtime_error("D3D12 runtime must be on the executable drive");
    auto path = relative.u8string();
    path += u8"\\";

    using GetInterface = HRESULT (WINAPI *)(REFCLSID, REFIID, void**);
    auto get = library.Get<GetInterface>("D3D12GetInterface");
    constexpr GUID classId{0x7CDA6ACA, 0xA03E, 0x49C8, {0x94, 0x58, 0x03, 0x34, 0xD2, 0x0E, 0x07, 0xCE}};
    constexpr GUID interfaceId{0xE9EB5314, 0x33AA, 0x42B2, {0xA7, 0x18, 0xD7, 0x7F, 0x58, 0xB1, 0xF1, 0xC7}};
    void* output = nullptr;
    Windows::CheckHResult(get(classId, interfaceId, &output), "D3D12 SDK configuration query");
    if (!output) throw std::runtime_error("D3D12 SDK configuration interface is null");
    auto release = [](ID3D12SDKConfiguration* value) { value->Release(); };
    std::unique_ptr<ID3D12SDKConfiguration, decltype(release)> configuration(static_cast<ID3D12SDKConfiguration*>(output), release);
    Windows::CheckHResult(configuration->SetSDKVersion(ANYBOX_AGILITY_SDK_VERSION, reinterpret_cast<const char*>(path.c_str())), "D3D12 SDK activation requires Windows Developer Mode or executable SDK exports");
}
