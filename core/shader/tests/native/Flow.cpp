#include <anybox/shader/NativeCompute.hpp>
#include <anybox/shader/ComputeCompiler.hpp>
#include "../buffer/Target.hpp"
#include "NativeFixtures.hpp"
#include <Recompiler.hpp>
#include <array>

namespace {
void Check(std::span<const std::uint32_t> words, bool loop) {
    using namespace Fixtures;
    using namespace AnyBox::Shader;
    auto data = MakeComputeProgram();
    Require(words.size() <= 6);
    for (std::size_t index = 0; index < words.size(); ++index) Set(data, 0x168 + index * 4, words[index]);
    NativeFrontend frontend(data);
    Require(!frontend.GetGraph().unsupported);
    if (loop) Require(!frontend.GetGraph().naturalLoops.empty());
    bool structured = false;
    for (const auto& block : frontend.GetGraph().blocks) structured |= block.terminator.mergeBlock != ShaderRecompiler::InvalidControlFlowId;
    Require(structured);
    auto state = AnyBox::Graphics::Container::ReadNativeComputeState(data);
    auto target = BufferTests::Target();
    auto translated = LowerNativeCompute(frontend, state, ShaderRecompiler::ShaderFloatMode{0xC0, true, true, false}, target);
    auto result = CompileCompute(translated.program, translated.input, target, {0, 0, 0, 128});
    bool merge = false;
    for (std::size_t offset = 5; offset < result.code.size();) {
        auto size = result.code[offset] >> 16;
        Require(size != 0 && size <= result.code.size() - offset);
        auto opcode = result.code[offset] & 65535;
        merge |= opcode == static_cast<unsigned>(loop ? spv::OpLoopMerge : spv::OpSelectionMerge);
        offset += size;
    }
    Require(merge);
}
}

void CheckNativeFlow() {
    Check(std::array<std::uint32_t, 6>{0xBF068004, 0xBF840002, 0xBE800381, 0xBF820001, 0xBE800382, 0xBF810000}, false);
    Check(std::array<std::uint32_t, 6>{0xBE800380, 0xBF0A0400, 0xBF840002, 0x80008100, 0xBF82FFFC, 0xBF810000}, true);
}
