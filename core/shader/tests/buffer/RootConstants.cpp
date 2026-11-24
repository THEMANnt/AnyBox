#include "ConstantFixture.hpp"
#include "Fixtures.hpp"
#include "Target.hpp"
#include "arguments/RootBindings.hpp"
#include "ComputeCompiler.hpp"
#include <anybox/shader/RootArgumentWriter.hpp>

namespace {
class Resources : public AnyBox::Shader::IRootResources {
public:
    AnyBox::Shader::RootBufferValue ResolveDescriptor(const AnyBox::Shader::BufferBinding& binding, std::uint64_t address) override {
        Fixtures::Require(binding.resourceClass == AnyBox::Shader::ResourceClass::UnorderedAccess && address == 0x200000000);
        ++calls;
        return {address, 240};
    }
    AnyBox::Shader::RootBufferValue ResolveTable(const AnyBox::Shader::BufferBinding&, std::uint32_t, std::uint32_t) override { throw std::runtime_error("Unexpected root constant test descriptor table"); }
    unsigned GetCallCount() const { return calls; }

private:
    unsigned calls = 0;
};
}

void BufferTests::CheckRootConstants(AnyBox::Vulkan::Context& context) {
    using namespace AnyBox::Shader;
    using namespace AnyBox::Graphics::Container;
    for (unsigned mode = 0; mode < 4; ++mode) {
        auto byteSize = (mode & 1) == 0 ? 52u : 64u;
        ConstantFixture fixture(false, true, byteSize, mode >= 2);
        RootSignatureLayout root{};
        root.version = 2;
        RootParameter output{};
        output.kind = RootParameterKind::UnorderedAccess;
        output.shaderRegister = 4;
        output.space = 2;
        RootParameter constants{};
        constants.kind = RootParameterKind::Constants;
        constants.shaderRegister = 4;
        constants.space = 2;
        constants.wordCount = byteSize == 52 ? 14 : 16;
        root.parameters = {output, constants};
        RootArgumentPlan plan(fixture.GetBindings(), root);
        Dxil::RootBindings bindings(plan);
        auto program = fixture.Lower(&bindings, plan.GetUserRegisterCount());
        ShaderRecompiler::ShaderComputeInputInfo input;
        input.threadsNum[0] = input.threadsNum[1] = input.threadsNum[2] = 1;
        input.waveSize = input.hostSubgroupSize = 32;
        auto shader = Dxil::CompileCompute(program, input, Target(), {0, 0, 0, 128});

        AnyBox::Graphics::Commands::RootState state(root);
        std::array descriptor{0u, 2u};
        state.Write(0, 0, descriptor);
        std::vector<std::uint32_t> values(byteSize / 4);
        auto constantBytes = std::as_writable_bytes(std::span(values));
        for (unsigned byte = 0; byte < constantBytes.size(); ++byte) constantBytes[byte] = static_cast<std::byte>((byte * 3 + 7) & 255);
        state.Write(1, 0, values);
        const auto& registers = shader.bindings.layout.userDataRegisters;
        RootArgumentWriter writer(plan, registers);
        std::vector<std::uint32_t> packed(writer.GetWordCount());
        Resources resources;
        writer.Write(state, resources, packed);
        Fixtures::Require(resources.GetCallCount() == 1);
        std::vector<std::uint32_t> userData(plan.GetUserRegisterCount());
        for (std::size_t index = 0; index < registers.size(); ++index) userData[registers[index]] = packed[index];
        std::array<std::uint32_t, 60> expected{};
        auto bytes = std::as_writable_bytes(std::span(expected));
        for (unsigned kind = 0; kind < 3; ++kind) {
            auto validBytes = kind == 1 ? byteSize / 8 * 8 : byteSize;
            for (unsigned byte = 0; byte < validBytes; ++byte) bytes[kind * 80 + byte] = static_cast<std::byte>((byte * 3 + 7) & 255);
        }
        RunGpu(context, shader, expected, {1, 1, 1}, {}, userData);
    }
}
