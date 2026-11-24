#pragma once

#include "Tests.hpp"
#include <anybox/shader/Artifact.hpp>
#include <filesystem>
#include <fstream>

namespace AnyBox::Graphics::Tests {
class PreparedCache {
public:
    PreparedCache() : directory(std::filesystem::current_path() / ("prepared-pso-" + std::to_string(GetCurrentProcessId()))) {
        auto size = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", nullptr, 0);
        if (size != 0) {
            previous.resize(size);
            auto length = GetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", previous.data(), size);
            Require(length != 0 && length < size, "Prepared cache test environment changed unexpectedly");
            previous.resize(length);
        }
        Require(std::filesystem::create_directory(directory), "Prepared cache test directory already exists");
        Require(SetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", directory.c_str()) != 0, "Prepared cache environment could not be set");
    }
    ~PreparedCache() {
        SetEnvironmentVariableW(L"ANYBOX_SHADER_CACHE", previous.empty() ? nullptr : previous.c_str());
        std::error_code error;
        if (!file.empty()) std::filesystem::remove(file, error);
        std::filesystem::remove(directory, error);
    }
    void Save(const Shader::ComputeArtifact& artifact) {
        file = directory / (Shader::DigestName(artifact.sourceHash) + ".abxc");
        auto bytes = Shader::EncodeComputeArtifact(artifact);
        std::ofstream output(file, std::ios::binary | std::ios::trunc);
        Require(static_cast<bool>(output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()))), "Prepared shader fixture write failed");
        output.close();
        Require(static_cast<bool>(output), "Prepared shader fixture close failed");
    }

private:
    std::wstring previous;
    std::filesystem::path directory;
    std::filesystem::path file;
};
}
