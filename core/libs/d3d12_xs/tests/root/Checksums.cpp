#include <d3d12_xs.hpp>
#include <anybox/graphics/container/Dxbc.hpp>
#include <d3d12.h>
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

namespace {
struct Release {
    void operator()(ID3DBlob* value) const noexcept { value->Release(); }
};

void CheckHostChecksums() {
    std::array<D3D12_ROOT_PARAMETER1, 32> parameters{};
    for (std::uint32_t index = 0; index < parameters.size(); ++index) {
        parameters[index].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        parameters[index].Constants = {index, 0, 1};
    }

    for (std::uint32_t count = 0; count <= parameters.size(); ++count) {
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
        description.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        description.Desc_1_1 = {count, parameters.data(), 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE};
        ID3DBlob* data = nullptr;
        D3D12SerializeVersionedRootSignature(&description, &data, nullptr);
        std::unique_ptr<ID3DBlob, Release> blob(data);
        auto bytes = std::span(static_cast<const std::byte*>(blob->GetBufferPointer()), blob->GetBufferSize());
        auto checksum = AnyBox::Graphics::Container::ComputeDxbcChecksum(bytes.subspan(20));
        if (!std::ranges::equal(checksum, bytes.subspan(4, 16))) throw std::runtime_error("DXBC checksum differs from the Windows serializer");
    }
}

void CheckXboxChecksums() {
    constexpr std::array<std::uint32_t, 5> flags{0, 0x01000000, 0x02000000, 0x03000000, 0x80000000};
    constexpr std::array<std::array<std::uint32_t, 4>, 5> expected{{
        {0x791882CB, 0x83C1DB39, 0x327EDC93, 0x3163085B},
        {0xD31772CF, 0x1C4E0DC4, 0x76D70A59, 0xA14BE9FE},
        {0x9A9D93C8, 0x1C152A66, 0x25EC29BE, 0x68FE6485},
        {0x4627CCF3, 0x1DE90F24, 0xF14B696B, 0xDDEB1D3E},
        {0x77839D81, 0x4106B681, 0xC35045A0, 0x264C8312}
    }};
    for (std::size_t index = 0; index < flags.size(); ++index) {
        std::array<std::uint32_t, 12> words{1, 68, 1, 36, 0x30535452, 24, 2, 0, 24, 0, 24, flags[index]};
        std::array<std::byte, 48> body{};
        for (std::size_t byte = 0; byte < body.size(); ++byte) body[byte] = static_cast<std::byte>(words[byte / 4] >> ((byte % 4) * 8));
        auto checksum = AnyBox::Graphics::Container::ComputeDxbcChecksum(body);
        for (std::size_t byte = 0; byte < checksum.size(); ++byte) {
            auto expectedByte = static_cast<std::byte>(expected[index][byte / 4] >> ((byte % 4) * 8));
            if (checksum[byte] != expectedByte) throw std::runtime_error("DXBC checksum differs from the native Xbox serializer");
        }
    }
}
}

void CheckDxbcChecksums() {
    CheckHostChecksums();
    CheckXboxChecksums();
}
