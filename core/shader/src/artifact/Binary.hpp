#pragma once

#include <anybox/shader/Artifact.hpp>
#include <stdexcept>

namespace AnyBox::Shader::ArtifactFormat {
inline constexpr std::size_t HeaderSize = 80;
inline constexpr std::array<std::byte, 8> Magic{std::byte{'A'}, std::byte{'B'}, std::byte{'X'}, std::byte{'C'}, std::byte{'S'}, std::byte{'0'}, std::byte{'0'}, std::byte{'1'}};
void Validate(const ComputeArtifact& artifact);

class Writer {
public:
    void Word(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) data.push_back(static_cast<std::byte>((value >> shift) & 255));
    }
    void Bytes(std::span<const std::byte> bytes) { data.insert(data.end(), bytes.begin(), bytes.end()); }
    void Words(std::span<const std::uint32_t> words) {
        if (words.size() > UINT32_MAX) throw std::length_error("Shader artifact word array is too large");
        Word(static_cast<std::uint32_t>(words.size()));
        for (auto word : words) Word(word);
    }
    std::vector<std::byte> Take() { return std::move(data); }

private:
    std::vector<std::byte> data;
};

class Reader {
public:
    explicit Reader(std::span<const std::byte> data) : data(data) {}
    std::span<const std::byte> Bytes(std::size_t count) {
        if (count > data.size()) throw std::invalid_argument("Shader artifact is truncated");
        auto result = data.first(count);
        data = data.subspan(count);
        return result;
    }
    std::uint32_t Word() {
        auto bytes = Bytes(4);
        std::uint32_t value = 0;
        for (unsigned index = 0; index < 4; ++index) value |= std::to_integer<std::uint32_t>(bytes[index]) << (index * 8);
        return value;
    }
    std::uint32_t Count(std::uint32_t minimumWords) {
        auto value = Word();
        if (value > data.size() / 4 / minimumWords) throw std::invalid_argument("Shader artifact array exceeds its payload");
        return value;
    }
    std::vector<std::uint32_t> Words() {
        auto count = Count(1);
        std::vector<std::uint32_t> result;
        result.reserve(count);
        for (std::uint32_t index = 0; index < count; ++index) result.push_back(Word());
        return result;
    }
    bool Empty() const noexcept { return data.empty(); }

private:
    std::span<const std::byte> data;
};
}
