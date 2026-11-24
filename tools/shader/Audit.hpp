#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace AnyBox::Shader::Tools {
std::size_t AuditCompute(std::span<const std::byte> code);
int AuditStream();
void SetBinaryInput();
}
