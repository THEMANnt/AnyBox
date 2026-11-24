#include "Dxgi.hpp"
#include <anybox/platform/WindowsError.hpp>
#include <stdexcept>

namespace {
class DxgiLibrary {
public:
    DxgiLibrary() : module(LoadLibraryExW(L"dxgi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32)), factory(nullptr) {
        if (!module) AnyBox::Windows::ThrowLastError("LoadLibraryExW DXGI");
        try {
            auto address = GetProcAddress(module, "CreateDXGIFactory2");
            if (!address) AnyBox::Windows::ThrowLastError("GetProcAddress CreateDXGIFactory2");
            using Create = HRESULT (WINAPI *)(UINT, REFIID, void**);
            Create create;
            static_assert(sizeof(create) == sizeof(address));
            std::memcpy(&create, &address, sizeof(create));
            constexpr GUID interfaceId{0x1BC6EA02, 0xEF36, 0x464F, {0xBF, 0x0C, 0x21, 0xCA, 0x39, 0xE5, 0x16, 0x8A}};
            void* output = nullptr;
            AnyBox::Windows::CheckHResult(create(0, interfaceId, &output), "CreateDXGIFactory2");
            if (!output) throw std::runtime_error("DXGI factory interface is null");
            factory = static_cast<IDXGIFactory4*>(output);
            HMODULE pinned = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<const wchar_t*>(module), &pinned)) AnyBox::Windows::ThrowLastError("GetModuleHandleExW DXGI lifetime");
        } catch (...) {
            if (factory) factory->Release();
            FreeLibrary(module);
            throw;
        }
    }
    ~DxgiLibrary() { factory->Release(); FreeLibrary(module); }
    IDXGIFactory4* GetFactory() const noexcept { return factory; }
    DxgiLibrary(const DxgiLibrary&) = delete;
    DxgiLibrary& operator=(const DxgiLibrary&) = delete;

private:
    HMODULE module;
    IDXGIFactory4* factory;
};
}

AnyBox::Graphics::NativeAdapter AnyBox::Graphics::GetHostAdapter(Device* device) {
    static DxgiLibrary library;
    constexpr GUID interfaceId{0x29038F61, 0x3839, 0x4626, {0x91, 0xFD, 0x08, 0x68, 0x79, 0x01, 0x1A, 0x05}};
    void* output = nullptr;
    Windows::CheckHResult(library.GetFactory()->EnumAdapterByLuid(device->GetHost()->GetAdapterLuid(), interfaceId, &output), "DXGI adapter lookup by device LUID");
    if (!output) throw std::runtime_error("DXGI adapter interface is null");
    return NativeAdapter(static_cast<IDXGIAdapter1*>(output));
}
