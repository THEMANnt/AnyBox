#include "Tests.hpp"
#include <d3dcompiler.h>
#include <memory>
#include <vector>

std::vector<std::byte> AnyBox::Graphics::Tests::CompileComputeShader() {
    constexpr auto source = R"(RWByteAddressBuffer output : register(u0);
[numthreads(1, 1, 1)]
void main() { output.Store(0, 0x12345678); }
)";
    ID3DBlob* bytecode = nullptr;
    ID3DBlob* errors = nullptr;
    auto result = D3DCompile(source, std::strlen(source), nullptr, nullptr, nullptr, "main", "cs_5_1", D3DCOMPILE_ENABLE_STRICTNESS, 0, &bytecode, &errors);
    auto release = [](ID3DBlob* value) { value->Release(); };
    std::unique_ptr<ID3DBlob, decltype(release)> blob(bytecode, release), diagnostics(errors, release);
    if (result != S_OK || !blob) throw std::runtime_error("Synthetic HLSL compute shader compilation failed");
    auto* first = static_cast<const std::byte*>(blob->GetBufferPointer());
    return {first, first + blob->GetBufferSize()};
}
