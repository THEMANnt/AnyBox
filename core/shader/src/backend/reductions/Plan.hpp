#pragma once

#include "Graph.hpp"
#include <optional>

namespace AnyBox::Shader::Reductions {
struct Plan {
    spv::Op operation;
    std::uint32_t scope;
    std::uint32_t source;
};
std::optional<Plan> Find(const Graph& graph, std::uint32_t result);
}
