#include <anybox/graphics/container/View.hpp>
#include <anybox/graphics/container/Dxbc.hpp>
#include "Integer.hpp"
#include <algorithm>
#include <cstring>

AnyBox::Graphics::Container::DxbcView::DxbcView(std::span<const std::byte> data) : data(data), count(0) {
    if (data.size() < 32 || std::memcmp(data.data(), "DXBC", 4) != 0) throw std::invalid_argument("DXBC container header is invalid");
    if (ReadInteger(data, 20) != 1 || ReadInteger(data, 24) != data.size()) throw std::invalid_argument("DXBC container version or length is invalid");
    count = ReadInteger(data, 28);
    if (count > (data.size() - 32) / 4) throw std::invalid_argument("DXBC container part table is invalid");
    auto checksum = ComputeDxbcChecksum(data.subspan(20));
    if (!std::ranges::equal(checksum, data.subspan(4, 16))) throw std::invalid_argument("DXBC container checksum is invalid");

    auto tableEnd = 32 + static_cast<std::size_t>(count) * 4;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto offset = static_cast<std::size_t>(ReadInteger(data, 32 + static_cast<std::size_t>(index) * 4));
        if (offset < tableEnd || offset % 4 != 0 || offset > data.size() - 8) throw std::invalid_argument("DXBC container part offset is invalid");
        auto size = ReadInteger(data, offset + 4);
        if (size > data.size() - offset - 8) throw std::invalid_argument("DXBC container part length is invalid");
        for (std::uint32_t previous = 0; previous < index; ++previous) {
            auto other = static_cast<std::size_t>(ReadInteger(data, 32 + static_cast<std::size_t>(previous) * 4));
            auto otherEnd = other + 8 + ReadInteger(data, other + 4);
            if (offset < otherEnd && other < offset + 8 + size) throw std::invalid_argument("DXBC container parts overlap");
        }
    }
}

AnyBox::Graphics::Container::Part AnyBox::Graphics::Container::DxbcView::GetPart(std::uint32_t index) const {
    if (index >= count) throw std::out_of_range("DXBC part index is outside the container");
    auto offset = ReadInteger(data, 32 + static_cast<std::size_t>(index) * 4);
    return {ReadInteger(data, offset), data.subspan(static_cast<std::size_t>(offset) + 8, ReadInteger(data, offset + 4))};
}

std::optional<AnyBox::Graphics::Container::Part> AnyBox::Graphics::Container::DxbcView::FindPart(std::uint32_t tag) const {
    std::optional<Part> result;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto part = GetPart(index);
        if (part.tag != tag) continue;
        if (result) throw std::invalid_argument("DXBC container has duplicate requested parts");
        result = part;
    }
    return result;
}
