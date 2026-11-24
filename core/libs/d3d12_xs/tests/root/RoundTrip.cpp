#include <d3d12_xs.hpp>
#include <anybox/ExceptionTest.hpp>
#include <anybox/platform/Windows.hpp>
#include <d3d12.h>
#include <array>
#include <memory>
#include <stdexcept>

void CheckDxbcChecksums();
void CheckXboxRootFlags();

namespace {
constexpr GUID DeserializerId{0x34AB647B, 0x3CC8, 0x46AC, {0x84, 0x1B, 0xC0, 0x96, 0x56, 0x45, 0xC0, 0x46}};

struct Release {
    template<class TInterface>
    void operator()(TInterface* value) const noexcept { value->Release(); }
};

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

template<class TException, class TCallable>
void Reject(TCallable&& callable) {
    int destructors = 0;
    bool caught = false;
    try { AnyBox::Tests::Invoke(callable, destructors); } catch (const TException&) { caught = true; }
    Require(caught && destructors == 1, "Root signature error did not unwind its caller");
}

void RoundTrip(D3D_ROOT_SIGNATURE_VERSION version) {
    D3D12_ROOT_PARAMETER parameter{};
    parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    parameter.Constants = {3, 2, 4};
    D3D12_ROOT_PARAMETER1 parameter1{};
    parameter1.ParameterType = parameter.ParameterType;
    parameter1.ShaderVisibility = parameter.ShaderVisibility;
    parameter1.Constants = parameter.Constants;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC description{};
    description.Version = version;
    if (version == D3D_ROOT_SIGNATURE_VERSION_1_0) description.Desc_1_0 = {1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    else description.Desc_1_1 = {1, &parameter1, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT};
    ID3DBlob* blob = nullptr;
    ID3DBlob* diagnostics = nullptr;
    Require(D3D12SerializeVersionedRootSignature(&description, &blob, &diagnostics) == 0, "Root signature serialization failed");
    std::unique_ptr<ID3DBlob, Release> ownedBlob(blob), ownedDiagnostics(diagnostics);
    Require(blob && blob->GetBufferSize() > 0 && !diagnostics, "Root signature serializer returned invalid outputs");
    void* object = nullptr;
    Require(D3D12CreateRootSignatureDeserializer(blob->GetBufferPointer(), blob->GetBufferSize(), DeserializerId, &object) == 0, "Root signature deserialization failed");
    std::unique_ptr<ID3D12RootSignatureDeserializer, Release> deserializer(static_cast<ID3D12RootSignatureDeserializer*>(object));
    auto* restored = deserializer->GetRootSignatureDesc();
    Require(restored && restored->NumParameters == 1 && restored->NumStaticSamplers == 0, "Root signature layout changed");
    Require(restored->Flags == D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT, "Root signature flags changed");
    const auto& value = restored->pParameters[0];
    Require(value.ParameterType == parameter.ParameterType && value.ShaderVisibility == parameter.ShaderVisibility, "Root parameter type or visibility changed");
    Require(value.Constants.Num32BitValues == 4 && value.Constants.ShaderRegister == 3 && value.Constants.RegisterSpace == 2, "Root constants changed");
    GUID unknown{};
    object = blob;
    Reject<std::system_error>([&] { D3D12CreateRootSignatureDeserializer(blob->GetBufferPointer(), blob->GetBufferSize(), unknown, &object); });
    Require(object == nullptr, "Failed root signature query retained a stale interface");
}

void Errors() {
    ID3DBlob* blob = nullptr;
    ID3DBlob* diagnostics = nullptr;
    Reject<std::invalid_argument>([&] { D3D12SerializeVersionedRootSignature(nullptr, &blob, &diagnostics); });
    Reject<std::invalid_argument>([&] { D3D12SerializeVersionedRootSignature(nullptr, nullptr, nullptr); });
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC invalid{};
    for (auto version : {0U, 3U, 99U, UINT32_MAX}) {
        invalid.Version = static_cast<D3D_ROOT_SIGNATURE_VERSION>(version);
        Reject<std::invalid_argument>([&] { D3D12SerializeVersionedRootSignature(&invalid, &blob, &diagnostics); });
        Require(!blob && !diagnostics, "Failed root signature serialization retained outputs");
    }
    std::array<std::byte, 16> bytes{};
    void* object = bytes.data();
    Reject<std::invalid_argument>([&] { D3D12CreateRootSignatureDeserializer(nullptr, 0, DeserializerId, &object); });
    Require(object == nullptr, "Invalid root signature input retained an interface");
    Reject<std::invalid_argument>([&] { D3D12CreateRootSignatureDeserializer(bytes.data(), bytes.size(), DeserializerId, nullptr); });
    Reject<std::system_error>([&] { D3D12CreateRootSignatureDeserializer(bytes.data(), bytes.size(), DeserializerId, &object); });
    Require(object == nullptr, "Corrupt root signature retained an interface");
}
}

int main() {
    return AnyBox::Tests::Run([] {
        RoundTrip(D3D_ROOT_SIGNATURE_VERSION_1_0);
        RoundTrip(D3D_ROOT_SIGNATURE_VERSION_1_1);
        Errors();
        CheckDxbcChecksums();
        CheckXboxRootFlags();
    });
}
