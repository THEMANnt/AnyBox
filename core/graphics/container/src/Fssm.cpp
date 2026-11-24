#include <anybox/graphics/container/Native.hpp>
#include "Integer.hpp"

namespace {
std::uint64_t ReadWide(std::span<const std::byte> data, std::size_t offset) {
    using AnyBox::Graphics::Container::ReadInteger;
    return ReadInteger(data, offset) | (static_cast<std::uint64_t>(ReadInteger(data, offset + 4)) << 32);
}
}

AnyBox::Graphics::Container::FssmView::FssmView(std::span<const std::byte> data) {
    if (data.size() < 0x138 || ReadInteger(data, 0) != 0x4D535346) throw std::invalid_argument("Xbox native shader program header is invalid");
    if (ReadInteger(data, 4) != 0x101) throw std::runtime_error("Xbox native shader program version is not implemented");
    codeSectionOffset = ReadWide(data, 0xC0);
    codeSectionSize = ReadWide(data, 0xC8);
    if (codeSectionOffset < 0x138 || codeSectionOffset % 4 != 0 || codeSectionSize < 44 || codeSectionSize % 4 != 0) throw std::invalid_argument("Xbox native shader code section is invalid");
    if (codeSectionOffset > data.size() || codeSectionSize > data.size() - codeSectionOffset) throw std::invalid_argument("Xbox native shader code section is truncated");
    waveSize = (ReadInteger(data, 0xD4) & 0x20) != 0 ? 32 : 64;
    code = data.subspan(static_cast<std::size_t>(codeSectionOffset) + 40, static_cast<std::size_t>(codeSectionSize) - 40);
}
