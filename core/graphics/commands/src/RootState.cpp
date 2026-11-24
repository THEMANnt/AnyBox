#include <anybox/graphics/commands/RootState.hpp>
#include <algorithm>
#include <cstring>
#include <stdexcept>

using namespace AnyBox::Graphics::Commands;
using namespace AnyBox::Graphics::Container;

RootState::RootState(const RootSignatureLayout& layout) {
    if (layout.version != 1 && layout.version != 2) throw std::invalid_argument("Root state signature version is invalid");
    if ((layout.flags & ~0x83000FFFU) != 0 || (layout.flags & 0x80) != 0) throw std::invalid_argument("Root state signature flags are invalid for command list binding");
    std::uint32_t remaining = 64;
    for (const auto& parameter : layout.parameters) {
        if (static_cast<unsigned>(parameter.kind) > 4) throw std::invalid_argument("Root state parameter kind is invalid");
        auto count = parameter.kind == RootParameterKind::Constants ? parameter.wordCount : parameter.kind == RootParameterKind::Table ? 1u : 2u;
        if (count == 0) throw std::invalid_argument("Root state constant parameter is empty");
        if (count > remaining) throw std::length_error("Root state signature exceeds the 64-word root cost limit");
        remaining -= count;
        parameters.push_back({parameter.kind, std::vector<std::uint32_t>(count), std::vector<bool>(count)});
    }
}

RootParameterKind RootState::GetKind(std::uint32_t parameter) const {
    if (parameter >= parameters.size()) throw std::out_of_range("Root state parameter index is outside the signature");
    return parameters[parameter].kind;
}

std::uint32_t RootState::GetWordCount(std::uint32_t parameter) const {
    static_cast<void>(GetKind(parameter));
    return static_cast<std::uint32_t>(parameters[parameter].words.size());
}

std::span<const std::uint32_t> RootState::Read(std::uint32_t parameter, std::uint32_t offset, std::uint32_t count) const {
    auto size = GetWordCount(parameter);
    if (offset > size || count > size - offset) throw std::out_of_range("Root state read exceeds the parameter size");
    const auto& selected = parameters[parameter];
    for (std::uint32_t index = offset; index < offset + count; ++index) if (!selected.initialized[index]) throw std::logic_error("Root state reads an uninitialized argument");
    return std::span(selected.words).subspan(offset, count);
}

void RootState::Write(std::uint32_t parameter, std::uint32_t offset, std::span<const std::uint32_t> words) {
    auto size = GetWordCount(parameter);
    if (offset > size || words.size() > size - offset) throw std::out_of_range("Root state write exceeds the parameter size");
    auto& selected = parameters[parameter];
    if (selected.kind != RootParameterKind::Constants && (offset != 0 || words.size() != size)) throw std::invalid_argument("Root descriptor and table updates must replace the entire argument");
    if (!words.empty()) std::memmove(selected.words.data() + offset, words.data(), words.size_bytes());
    std::fill_n(selected.initialized.begin() + offset, words.size(), true);
}

void RootState::Invalidate() noexcept {
    for (auto& parameter : parameters) std::fill(parameter.initialized.begin(), parameter.initialized.end(), false);
}
