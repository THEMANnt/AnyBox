#pragma once

#include "PrivateDataSlot.hpp"
#include <cwchar>
#include <limits>

namespace AnyBox::Graphics {
template<class TObject>
class ManagedProperties {
public:
    static HRESULT GetPrivateData(TObject* self, const Guid& key, std::uint32_t* size, void* data) {
        auto result = self->GetPrivateDataStore().Get(key, size, data);
        CheckGraphicsResult(result, "Get graphics object private data");
        return result;
    }
    static HRESULT SetPrivateData(TObject* self, const Guid& key, std::uint32_t size, const void* data) { return self->GetPrivateDataStore().Set(key, size, data); }
    static HRESULT SetPrivateDataInterface(TObject* self, const Guid& key, const IUnknown* object) { return self->GetPrivateDataStore().SetInterface(key, object); }
    static HRESULT SetName(TObject* self, const wchar_t* name) {
        if (!name) throw std::invalid_argument("Graphics object name is null");
        auto length = std::wcslen(name);
        if (length >= std::numeric_limits<std::uint32_t>::max() / sizeof(wchar_t)) throw std::length_error("Graphics object name is too long");
        constexpr Guid nameId{0x4CCA5FD8, 0x921F, 0x42C8, {0x85, 0x66, 0x70, 0xCA, 0xF2, 0xA9, 0xB7, 0x41}};
        return SetPrivateData(self, nameId, static_cast<std::uint32_t>((length + 1) * sizeof(wchar_t)), name);
    }
};
}
