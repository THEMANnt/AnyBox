#pragma once

#include "RootState.hpp"
#include <array>
#include <variant>

namespace AnyBox::Graphics::Commands {
struct RootWrite {
    std::uint32_t parameter;
    std::uint32_t offset;
    std::span<const std::uint32_t> words;
};

struct Dispatch {
    std::array<std::uint32_t, 3> groups;
    std::uint32_t header;
};

using InlineCommand = std::variant<RootWrite, Dispatch>;

std::uint32_t RootHeader(std::uint32_t parameter);

class InlineReader {
public:
    explicit InlineReader(std::span<const std::uint32_t> words) : words(words) {}
    bool Empty() const noexcept { return words.empty(); }
    InlineCommand Next(const RootState& root);

private:
    std::span<const std::uint32_t> words;
};
}
