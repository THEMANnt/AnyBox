#pragma once

#include <cstdint>
#include <spirv-tools/libspirv.h>

namespace AnyBox::Shader {
spv_target_env GetSpirvEnvironment(std::uint32_t version);
}
