#include <anybox/graphics/container/Dxil.hpp>
#include "Integer.hpp"

AnyBox::Graphics::Container::DxilView::DxilView(std::span<const std::byte> data) : data(data) {
    if (data.size() < 28 || data.size() % 4 != 0) throw std::invalid_argument("DXIL program size is invalid");
    if (static_cast<std::uint64_t>(ReadInteger(data, 4)) * 4 != data.size()) throw std::invalid_argument("DXIL program word count is invalid");
    if (ReadInteger(data, 8) != 0x4C495844) throw std::invalid_argument("DXIL program magic is invalid");
    programVersion = ReadInteger(data, 0);
    dxilVersion = ReadInteger(data, 12);
    auto offset = static_cast<std::uint64_t>(ReadInteger(data, 16)) + 8;
    auto size = ReadInteger(data, 20);
    if (offset < 24 || offset > data.size() || size < 4 || size > data.size() - offset) throw std::invalid_argument("DXIL bitcode range is invalid");
    auto payload = data.subspan(static_cast<std::size_t>(offset), size);
    auto magic = ReadInteger(payload, 0);
    if (magic == 0xDEC04342) bitcode = payload;
    else if (magic != 0x43424F4E || size != 4) throw std::invalid_argument("DXIL bitcode magic is invalid");
    if (offset < 36 || ReadInteger(data, 24) != 0x54414458) return;
    count = ReadInteger(data, 28);
    table = ReadInteger(data, 32);
    if (table < 36 || table % 4 != 0 || table > data.size() || count > (data.size() - table) / 12) throw std::invalid_argument("Xbox DXIL embedded data table is invalid");
    auto tableEnd = table + static_cast<std::uint64_t>(count) * 12;
    if (tableEnd > offset) throw std::invalid_argument("Xbox DXIL embedded data table overlaps the bitcode");

    for (std::uint32_t index = 0; index < count; ++index) {
        auto entry = table + static_cast<std::size_t>(index) * 12;
        auto start = ReadInteger(data, entry + 4);
        auto length = ReadInteger(data, entry + 8);
        if (start < tableEnd || start % 4 != 0 || start > data.size() || length > data.size() - start) throw std::invalid_argument("Xbox DXIL embedded data range is invalid");
        for (std::uint32_t previous = 0; previous < index; ++previous) {
            auto other = table + static_cast<std::size_t>(previous) * 12;
            auto otherStart = ReadInteger(data, other + 4);
            auto otherEnd = static_cast<std::uint64_t>(otherStart) + ReadInteger(data, other + 8);
            if (start < otherEnd && otherStart < static_cast<std::uint64_t>(start) + length) throw std::invalid_argument("Xbox DXIL embedded data ranges overlap");
        }
    }
}

std::span<const std::byte> AnyBox::Graphics::Container::DxilView::GetBitcode() const {
    if (!HasBitcode()) throw std::runtime_error("Native shader program has no DXIL bitcode");
    return bitcode;
}

AnyBox::Graphics::Container::Part AnyBox::Graphics::Container::DxilView::GetEmbeddedPart(std::uint32_t index) const {
    if (index >= count) throw std::out_of_range("Xbox DXIL embedded data index is invalid");
    auto entry = table + static_cast<std::size_t>(index) * 12;
    return {ReadInteger(data, entry), data.subspan(ReadInteger(data, entry + 4), ReadInteger(data, entry + 8))};
}

std::optional<AnyBox::Graphics::Container::Part> AnyBox::Graphics::Container::DxilView::FindEmbeddedPart(std::uint32_t kind) const {
    std::optional<Part> result;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto part = GetEmbeddedPart(index);
        if (part.tag != kind) continue;
        if (result) throw std::invalid_argument("Xbox DXIL has duplicate requested embedded data");
        result = part;
    }
    return result;
}
