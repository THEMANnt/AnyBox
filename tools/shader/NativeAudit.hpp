#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace AnyBox::Shader::Tools {
std::string InspectNative(std::span<const std::byte> container);
std::string InspectNativeIr(std::span<const std::byte> container);
std::size_t AuditNative(std::span<const std::byte> container);
}
