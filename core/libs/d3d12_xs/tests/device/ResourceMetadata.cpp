#include "Tests.hpp"
#include "ResourceMetadata.hpp"
#include "PrivateDataDiagnostics.hpp"
#include <system_error>

HRESULT AnyBox::Graphics::Tests::ReadResourceData(Resource* resource, const Guid& key, std::uint32_t* size, void* data) {
    try { return Method<decltype(&Resource::GetPrivateData)>(resource, 3)(resource, key, size, data); }
    catch (const std::system_error& error) { return static_cast<HRESULT>(error.code().value()); }
}

namespace {
void Compare(AnyBox::Graphics::Device* device, AnyBox::Graphics::Resource* resource, const AnyBox::Guid& key) {
    using namespace AnyBox::Graphics;
    using namespace AnyBox::Graphics::Tests;
    for (std::uint32_t capacity : {0u, 1u, 64u}) {
        for (bool query : {false, true}) {
            std::array<std::byte, 64> expected{}, actual{};
            auto size = capacity;
            auto expectedSize = capacity;
            auto result = ReadResourceData(resource, key, &size, query ? nullptr : actual.data());
            auto expectedResult = device->GetHost()->GetPrivateData(MetadataGuid(key), &expectedSize, query ? nullptr : expected.data());
            if (result != expectedResult || size != expectedSize || actual != expected) throw std::runtime_error("Resource private data differs from D3D12: key " + std::to_string(key.data1) + ", capacity " + std::to_string(capacity) + ", query " + std::to_string(query) + ", result " + std::to_string(result) + "/" + std::to_string(expectedResult) + ", size " + std::to_string(size) + "/" + std::to_string(expectedSize));
        }
    }
}
}

void AnyBox::Graphics::Tests::CheckResourceMetadata(Device* device, Resource* resource) {
    PrivateDataDiagnostics diagnostics(device->GetHost());
    constexpr Guid key{0x78B5EF71, 0x43A2, 0x4FDE, {0x8F, 0xAA, 0xA0, 0x25, 0x61, 0x67, 0x82, 0x05}};
    auto set = Method<decltype(&Resource::SetPrivateData)>(resource, 4);
    auto name = Method<decltype(&Resource::SetName)>(resource, 6);
    Compare(device, resource, key);
    std::array<std::byte, 17> bytes{};
    for (std::uint32_t count : {0u, 1u, 17u}) {
        for (std::size_t index = 0; index < bytes.size(); ++index) bytes[index] = static_cast<std::byte>(index * 7 + count);
        auto expected = device->GetHost()->SetPrivateData(MetadataGuid(key), count, bytes.data());
        Require(set(resource, key, count, bytes.data()) == expected, "Resource private data storage status differs from D3D12");
        bytes.fill(std::byte{0x99});
        Compare(device, resource, key);
    }
    for (int index = 0; index < 2; ++index) {
        auto expected = device->GetHost()->SetPrivateData(MetadataGuid(key), 0, nullptr);
        Require(set(resource, key, 0, nullptr) == expected, "Resource private data removal status differs from D3D12");
        Compare(device, resource, key);
    }

    constexpr Guid nameId{0x4CCA5FD8, 0x921F, 0x42C8, {0x85, 0x66, 0x70, 0xCA, 0xF2, 0xA9, 0xB7, 0x41}};
    for (const auto* value : {L"Xbox buffer", L"Буфер Ω", L""}) {
        Require(name(resource, value) == device->GetHost()->SetName(value), "Resource naming status differs from native D3D12");
        Compare(device, resource, nameId);
    }
    Reject<std::invalid_argument>([&] { name(resource, nullptr); });
    Reject<std::invalid_argument>([&] { set(resource, key, 1, nullptr); });
    Reject<std::invalid_argument>([&] { ReadResourceData(resource, key, nullptr, nullptr); });
    CheckResourceInterfaces(device, resource);
}
