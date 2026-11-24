#pragma once

#include <cstdint>
#include <vector>

namespace AnyBox::Shader {
bool RemoveDeadSubgroupOperations(std::vector<std::uint32_t>& code, std::uint32_t vulkanVersion);
}
