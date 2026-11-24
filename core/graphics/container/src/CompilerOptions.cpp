#include <anybox/graphics/container/Dxil.hpp>
#include "Integer.hpp"

AnyBox::Graphics::Container::CompilerOptionsView::CompilerOptionsView(std::span<const std::byte> data) : data(data), count(0) {
    if (data.size() < 8 || ReadInteger(data, 0) != 0x706F6278) throw std::invalid_argument("Xbox shader compiler options header is invalid");
    count = ReadInteger(data, 4);
    if (count > (data.size() - 8) / 8) throw std::invalid_argument("Xbox shader compiler option count is invalid");
    std::size_t offset = 8;
    for (std::uint32_t index = 0; index < count; ++index) {
        if (data.size() - offset < 8) throw std::invalid_argument("Xbox shader compiler option is truncated");
        auto length = ReadInteger(data, offset + 4);
        offset += 8;
        if (length > data.size() - offset) throw std::invalid_argument("Xbox shader compiler option value is truncated");
        offset += length;
    }
    if (offset != data.size()) throw std::invalid_argument("Xbox shader compiler options contain trailing data");
}

AnyBox::Graphics::Container::CompilerOption AnyBox::Graphics::Container::CompilerOptionsView::GetOption(std::uint32_t index) const {
    if (index >= count) throw std::out_of_range("Xbox shader compiler option index is invalid");
    std::size_t offset = 8;
    for (std::uint32_t current = 0; current < index; ++current) offset += 8 + static_cast<std::size_t>(ReadInteger(data, offset + 4));
    auto length = ReadInteger(data, offset + 4);
    return {ReadInteger(data, offset), {reinterpret_cast<const char*>(data.data() + offset + 8), length}};
}

std::optional<AnyBox::Graphics::Container::CompilerOption> AnyBox::Graphics::Container::CompilerOptionsView::FindOption(std::uint32_t key) const {
    std::optional<CompilerOption> result;
    for (std::uint32_t index = 0; index < count; ++index) {
        auto option = GetOption(index);
        if (option.key != key) continue;
        if (result) throw std::invalid_argument("Xbox shader compiler options contain a duplicate requested key");
        result = option;
    }
    return result;
}
