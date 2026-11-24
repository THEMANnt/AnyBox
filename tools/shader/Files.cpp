#include "Files.hpp"
#include <fstream>
#include <limits>
#include <stdexcept>

std::vector<std::byte> AnyBox::Shader::Tools::ReadInput(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open shader input");
    auto size = input.tellg();
    if (size <= 0 || static_cast<std::uint64_t>(size) > std::numeric_limits<std::uint32_t>::max()) throw std::length_error("Shader input size is invalid");
    std::vector<std::byte> data(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char*>(data.data()), size)) throw std::runtime_error("Cannot read shader input");
    return data;
}

void AnyBox::Shader::Tools::WriteOutput(const std::filesystem::path& path, std::span<const std::byte> bytes) {
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) throw std::length_error("Shader output exceeds the stream size limit");
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Cannot open shader output");
    if (!output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))) throw std::runtime_error("Cannot write shader output");
    output.close();
    if (!output) throw std::runtime_error("Cannot close shader output");
}
