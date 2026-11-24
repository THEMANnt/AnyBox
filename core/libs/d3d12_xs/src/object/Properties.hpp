#pragma once

#include <anybox/Types.hpp>
#include <anybox/platform/Windows.hpp>
#include <unknwn.h>
#include <stdexcept>

namespace AnyBox::Graphics {
inline constexpr Guid ObjectId{0xC4FEC28F, 0x7966, 0x4E95, {0x9F, 0x94, 0xF4, 0x31, 0xCB, 0x56, 0xC3, 0xB8}};
inline constexpr Guid DeviceChildId{0x905DB94B, 0xA00C, 0x4140, {0x9D, 0xF5, 0x2B, 0x64, 0xCA, 0x9E, 0xA3, 0x57}};
inline constexpr Guid PageableId{0x63EE58FB, 0x1268, 0x4835, {0x86, 0xDA, 0xF0, 0x08, 0xCE, 0x62, 0xF0, 0xD6}};
GUID NativeGuid(const Guid& value);
void CheckGraphicsResult(HRESULT result, const char* operation);

template<class TObject>
class ObjectProperties {
public:
    static HRESULT GetPrivateData(TObject* self, const Guid& key, std::uint32_t* size, void* data) {
        if (!size) throw std::invalid_argument("Graphics private data size output is null");
        auto result = self->GetHost()->GetPrivateData(NativeGuid(key), size, data);
        CheckGraphicsResult(result, "ID3D12Object::GetPrivateData");
        return result;
    }

    static HRESULT SetPrivateData(TObject* self, const Guid& key, std::uint32_t size, const void* data) {
        if (size != 0 && !data) throw std::invalid_argument("Graphics private data is null");
        auto result = self->GetHost()->SetPrivateData(NativeGuid(key), size, data);
        CheckGraphicsResult(result, "ID3D12Object::SetPrivateData");
        return result;
    }

    static HRESULT SetPrivateDataInterface(TObject* self, const Guid& key, const IUnknown* object) {
        auto result = self->GetHost()->SetPrivateDataInterface(NativeGuid(key), object);
        CheckGraphicsResult(result, "ID3D12Object::SetPrivateDataInterface");
        return result;
    }

    static HRESULT SetName(TObject* self, const wchar_t* name) {
        if (!name) throw std::invalid_argument("Graphics object name is null");
        auto result = self->GetHost()->SetName(name);
        CheckGraphicsResult(result, "ID3D12Object::SetName");
        return result;
    }
};
}
