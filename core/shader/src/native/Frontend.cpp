#include <anybox/shader/Native.hpp>
#include <anybox/graphics/container/Native.hpp>
#include "Decode.hpp"
#include <RdnaDecoder/RdnaInstructionDecoder.hpp>
#include <ControlFlow/GraphBuilder.hpp>
#include <ControlFlow/Structurizer.hpp>
#include <stdexcept>

AnyBox::Shader::NativeFrontend::NativeFrontend(std::span<const std::byte> program) {
    if (ShaderRecompiler::GcnGuestShaders()) throw std::logic_error("Xbox native program requires RDNA decoding mode");
    Graphics::Container::FssmView view(program);
    auto bytes = view.GetCode();
    waveSize = view.GetWaveSize();
    code.resize(bytes.size() / 4);
    for (std::size_t word = 0; word < code.size(); ++word) {
        for (unsigned byte = 0; byte < 4; ++byte) code[word] |= std::to_integer<std::uint32_t>(bytes[word * 4 + byte]) << (byte * 8);
    }
    decoded = DecodeNativeProgram(code);
    graph = ShaderRecompiler::GraphBuilder{}.Build(decoded);
    if (graph.unsupported) throw std::runtime_error(graph.unsupportedReason);
    ShaderRecompiler::Structurizer{}.Structurize(graph);
    if (graph.unsupported) throw std::runtime_error(graph.unsupportedReason);
}
