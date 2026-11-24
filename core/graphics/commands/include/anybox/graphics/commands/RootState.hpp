#pragma once

#include <anybox/graphics/container/RootSignature.hpp>

namespace AnyBox::Graphics::Commands {
class RootState {
public:
    explicit RootState(const Container::RootSignatureLayout& layout);
    Container::RootParameterKind GetKind(std::uint32_t parameter) const;
    std::uint32_t GetWordCount(std::uint32_t parameter) const;
    std::span<const std::uint32_t> Read(std::uint32_t parameter, std::uint32_t offset, std::uint32_t count) const;
    void Write(std::uint32_t parameter, std::uint32_t offset, std::span<const std::uint32_t> words);
    void Invalidate() noexcept;

private:
    struct Parameter {
        Container::RootParameterKind kind;
        std::vector<std::uint32_t> words;
        std::vector<bool> initialized;
    };
    std::vector<Parameter> parameters;
};
}
