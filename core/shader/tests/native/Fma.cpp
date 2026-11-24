#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <Recompiler.hpp>
#include <array>

namespace {
std::vector<std::byte> Make(std::span<const std::uint32_t> code) {
    auto data = Fixtures::MakeComputeProgram();
    auto bytes = (40 + code.size() * 4 + 15) & ~std::size_t{15};
    data.resize(0x140 + bytes);
    Fixtures::Set(data, 0xC8, static_cast<std::uint32_t>(bytes));
    for (std::size_t offset = 0x168; offset < data.size(); offset += 4) Fixtures::Set(data, offset, 0xBF9F0000);
    for (std::size_t index = 0; index < code.size(); ++index) Fixtures::Set(data, 0x168 + index * 4, code[index]);
    return data;
}

void Flow(std::span<const std::uint32_t> code) {
    using namespace AnyBox::Shader;
    auto data = Make(code);
    NativeFrontend frontend(data);
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    auto target = BufferTests::Target();
    auto lowered = LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{0xC0, true, false, false}, target);
    Fixtures::Require(lowered.program.Blocks().size() == frontend.GetGraph().blocks.size() + 1);
    auto result = CompileCompute(lowered.program, lowered.input, target, {0, 0, 0, 128});
    Fixtures::Require(result.code.size() > 5);
}
}

void CheckNativeFmaDecoding() {
    using namespace Fixtures;
    using namespace ShaderRecompiler;
    for (unsigned absolute = 0; absolute < 8; ++absolute) {
        for (unsigned negate = 0; negate < 8; ++negate) {
            std::array<std::uint32_t, 5> code{0xD5408003 | (absolute << 8), 0x18080200 | (negate << 29), 0xD5408004, 0x00080200, 0xBF810000};
            auto data = Make(code);
            AnyBox::Shader::NativeFrontend frontend(data);
            const auto& decoded = frontend.GetDecoded();
            Require(decoded.code.data() == frontend.GetCode().data());
            for (unsigned index = 0; index < 2; ++index) {
                const auto& instruction = decoded.instructions.at(index);
                Require(instruction.op == RdnaOpcode::VMadLegacyF32 && instruction.opcodeId == 0x140);
                Require(instruction.rawWords[0] == code[index * 2] && instruction.rawWords[1] == code[index * 2 + 1]);
                Require(instruction.destination.clamp && instruction.destination.omod == (index == 0 ? 3u : 0u));
            }
            const auto& instruction = decoded.instructions.front();
            std::array sources{instruction.source0, instruction.source1, instruction.source2};
            for (unsigned source = 0; source < sources.size(); ++source) {
                Require(sources[source].absolute == ((absolute & (1u << source)) != 0));
                Require(sources[source].negate == ((negate & (1u << source)) != 0));
            }
        }
    }
    auto invalid = Make(std::array<std::uint32_t, 3>{0xD5408803, 0x00080200, 0xBF810000});
    Reject<std::invalid_argument>([&] { AnyBox::Shader::NativeFrontend frontend(invalid); });
    Flow(std::array<std::uint32_t, 10>{0xBF068004, 0xBF850003, 0xD5408003, 0x00080200, 0xBF820002, 0xD5400003, 0x00080200, 0xDC708000, 0x007F0301, 0xBF810000});
    Flow(std::array<std::uint32_t, 8>{0xBE850380, 0xBF0A0405, 0xBF840004, 0xD5408003, 0x00080200, 0x80058105, 0xBF82FFFA, 0xBF810000});
}
