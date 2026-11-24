#pragma once

#include "View.hpp"
#include <string_view>

namespace AnyBox::Graphics::Container {
class DxilView {
public:
    explicit DxilView(std::span<const std::byte> data);
    std::span<const std::byte> GetBitcode() const;
    bool HasBitcode() const noexcept { return !bitcode.empty(); }
    std::uint32_t GetProgramVersion() const noexcept { return programVersion; }
    std::uint32_t GetDxilVersion() const noexcept { return dxilVersion; }
    bool HasEmbeddedData() const noexcept { return table != 0; }
    std::uint32_t GetEmbeddedPartCount() const noexcept { return count; }
    Part GetEmbeddedPart(std::uint32_t index) const;
    std::optional<Part> FindEmbeddedPart(std::uint32_t kind) const;

private:
    std::span<const std::byte> data;
    std::span<const std::byte> bitcode;
    std::uint32_t programVersion;
    std::uint32_t dxilVersion;
    std::uint32_t table = 0;
    std::uint32_t count = 0;
};

struct CompilerOption {
    std::uint32_t key;
    std::string_view value;
};

class CompilerOptionsView {
public:
    explicit CompilerOptionsView(std::span<const std::byte> data);
    std::uint32_t GetCount() const noexcept { return count; }
    CompilerOption GetOption(std::uint32_t index) const;
    std::optional<CompilerOption> FindOption(std::uint32_t key) const;

private:
    std::span<const std::byte> data;
    std::uint32_t count;
};
}
