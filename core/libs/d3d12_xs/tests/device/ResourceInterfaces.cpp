#include "Tests.hpp"
#include "ResourceMetadata.hpp"

namespace {
class Unknown : public IUnknown {
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void** object) override {
        if (!object) return E_POINTER;
        *object = nullptr;
        constexpr GUID unknownId{0, 0, 0, {0xC0, 0, 0, 0, 0, 0, 0, 0x46}};
        if (std::memcmp(&id, &unknownId, sizeof(id)) != 0) return E_NOINTERFACE;
        AddRef();
        *object = this;
        return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override { return --references; }
    ULONG GetReferences() const { return references; }

private:
    ULONG references = 1;
};
}

void AnyBox::Graphics::Tests::CheckResourceInterfaces(Device* device, Resource* resource) {
    constexpr Guid key{0x834AE71A, 0xD618, 0x4D22, {0xBE, 0x00, 0x6C, 0xF2, 0x93, 0xF9, 0xA9, 0x4F}};
    auto set = Method<decltype(&Resource::SetPrivateDataInterface)>(resource, 5);
    Unknown expected, actual;
    for (bool present : {true, true, false, false}) {
        auto result = set(resource, key, present ? &actual : nullptr);
        auto expectedResult = device->GetHost()->SetPrivateDataInterface(MetadataGuid(key), present ? &expected : nullptr);
        Require(result == expectedResult && actual.GetReferences() == expected.GetReferences(), "Resource private interface retention differs from D3D12");
        for (bool query : {true, false}) {
            IUnknown* actualOutput = nullptr;
            IUnknown* expectedOutput = nullptr;
            std::uint32_t size = sizeof(actualOutput), expectedSize = sizeof(expectedOutput);
            result = ReadResourceData(resource, key, &size, query ? nullptr : &actualOutput);
            expectedResult = device->GetHost()->GetPrivateData(MetadataGuid(key), &expectedSize, query ? nullptr : &expectedOutput);
            if (result != expectedResult || size != expectedSize || actual.GetReferences() != expected.GetReferences()) throw std::runtime_error("Resource private interface lookup differs from D3D12: present " + std::to_string(present) + ", query " + std::to_string(query) + ", result " + std::to_string(result) + "/" + std::to_string(expectedResult) + ", size " + std::to_string(size) + "/" + std::to_string(expectedSize) + ", refs " + std::to_string(actual.GetReferences()) + "/" + std::to_string(expected.GetReferences()));
            Require(actualOutput == (expectedOutput ? &actual : nullptr), "Resource private interface identity changed");
            if (actualOutput) actualOutput->Release();
            if (expectedOutput) expectedOutput->Release();
        }
    }
    Require(actual.GetReferences() == 1 && expected.GetReferences() == 1, "Resource private interface removal leaked a reference");
}
