#include "Choice.hpp"
#include <limits>

bool AnyBox::Shader::SameDescriptorValue(ShaderRecompiler::IrValue* first, ShaderRecompiler::IrValue* second) {
    first = first->Resolve();
    second = second->Resolve();
    return first == second || (first->HasImmediate() && second->HasImmediate() && first->Type() == second->Type() && first->ImmediateU64() == second->ImmediateU64());
}

std::optional<AnyBox::Shader::DescriptorRead> AnyBox::Shader::ReadDescriptor(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& read) {
    using namespace ShaderRecompiler;
    if (read.Opcode() != IrOpcode::LoadAddressU32 || read.ArgumentCount() != 4 || read.UseCount() != 1) return std::nullopt;
    auto flags = read.Flags<MemoryFlags>();
    const auto& memory = program.Resources().memoryInfo.at(flags.index);
    if (memory.kind != ResourceKind::ScalarAddress || memory.addressIsFull || memory.coherent || memory.dataBits != 32 || memory.dataDwords != 1) return std::nullopt;
    auto* address = read.Argument(0)->Resolve();
    auto* offset = read.Argument(1)->Resolve();
    auto* high = read.Argument(2)->Resolve();
    auto* active = read.Argument(3)->Resolve();
    if (address->Opcode() != IrOpcode::GetAddressResource || address->ArgumentCount() != 2) return std::nullopt;
    if (!offset->HasImmediate() || offset->Type() != IrType::U32 || !high->HasImmediate() || high->Type() != IrType::U32 || high->ImmediateU32() != 0) return std::nullopt;
    if (!active->HasImmediate() || active->Type() != IrType::Bool || !active->ImmediateBool()) return std::nullopt;
    auto total = static_cast<std::uint64_t>(memory.offset) + offset->ImmediateU32();
    if (total > std::numeric_limits<std::uint32_t>::max()) return std::nullopt;
    return DescriptorRead{address->Argument(0)->Resolve(), address->Argument(1)->Resolve(), memory, flags, static_cast<std::uint32_t>(total)};
}
