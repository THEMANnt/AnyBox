#pragma once

#include <anybox/relinker/Cpu.hpp>
#include <optional>

namespace AnyBox::Relinker {
std::optional<bool> GetScalarSupport(const CpuState& state, ZydisISASet isa);
std::optional<bool> GetVectorSupport(const CpuState& state, ZydisISASet isa);
}
