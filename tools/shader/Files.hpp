#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace AnyBox::Shader::Tools {
std::vector<std::byte> ReadInput(const std::filesystem::path& path);
void WriteOutput(const std::filesystem::path& path, std::span<const std::byte> bytes);
}
