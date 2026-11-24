#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace AnyBox::Graphics::Container {
struct Part {
    std::uint32_t tag;
    std::span<const std::byte> data;
};

class DxbcView {
public:
    explicit DxbcView(std::span<const std::byte> data);
    std::uint32_t GetPartCount() const noexcept { return count; }
    Part GetPart(std::uint32_t index) const;
    std::optional<Part> FindPart(std::uint32_t tag) const;

private:
    std::span<const std::byte> data;
    std::uint32_t count;
};

std::vector<std::byte> BuildDxbc(std::span<const Part> parts);
}
