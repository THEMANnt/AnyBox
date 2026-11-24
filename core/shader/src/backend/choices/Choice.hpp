#pragma once

#include <IntermediateRepresentation/IrBuilder.hpp>
#include <array>
#include <cstddef>
#include <optional>

namespace AnyBox::Shader {
struct DescriptorRead {
    ShaderRecompiler::IrValue* low = nullptr;
    ShaderRecompiler::IrValue* high = nullptr;
    ShaderRecompiler::MemoryInfo memory;
    ShaderRecompiler::MemoryFlags flags;
    std::uint32_t offset = 0;
};

struct DescriptorChoice {
    ShaderRecompiler::IrValue* condition = nullptr;
    std::array<std::array<DescriptorRead, 2>, 8> reads;
    std::size_t words = 0;
};

bool SameDescriptorValue(ShaderRecompiler::IrValue* first, ShaderRecompiler::IrValue* second);
std::optional<DescriptorRead> ReadDescriptor(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& read);
std::optional<DescriptorChoice> FindDescriptorChoice(ShaderRecompiler::IrProgram& program, ShaderRecompiler::IrValue& handle);
std::size_t PlanDescriptorChoices(ShaderRecompiler::IrProgram& program);
}
