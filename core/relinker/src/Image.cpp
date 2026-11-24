#include <anybox/relinker/Image.hpp>
#include <algorithm>
#include <limits>

AnyBox::Relinker::Image::Image(std::vector<std::byte> bytes) : bytes(std::move(bytes)) {
    auto data = GetBytes();
    if (Read<std::uint16_t>(data, 0) != 0x5A4D) throw std::runtime_error("Input has no DOS image header");
    auto pe = static_cast<std::size_t>(Read<std::uint32_t>(data, 0x3C));
    if (Read<std::uint32_t>(data, pe) != 0x4550 || Read<std::uint16_t>(data, pe + 4) != 0x8664) throw std::runtime_error("Input is not an AMD64 PE image");
    auto count = Read<std::uint16_t>(data, pe + 6);
    auto optionalSize = Read<std::uint16_t>(data, pe + 20);
    optionalHeader = pe + 24;
    if (count == 0 || count > 96 || optionalSize < 112 || Read<std::uint16_t>(data, optionalHeader) != 0x20B) throw std::runtime_error("PE image header is invalid");
    base = Read<std::uint64_t>(data, optionalHeader + 24);
    entry = Read<std::uint32_t>(data, optionalHeader + 16);
    headerSize = Read<std::uint32_t>(data, optionalHeader + 60);
    directoryCount = Read<std::uint32_t>(data, optionalHeader + 108);
    if (directoryCount > directories.size() || optionalSize < 112 + directoryCount * 8 || headerSize > data.size() || base > std::numeric_limits<std::uint64_t>::max() - UINT32_MAX) throw std::runtime_error("PE optional header bounds are invalid");
    for (std::size_t index = 0; index < directoryCount; ++index) {
        auto offset = optionalHeader + 112 + index * 8;
        directories[index] = {Read<std::uint32_t>(data, offset), Read<std::uint32_t>(data, offset + 4)};
        if ((directories[index].address == 0) != (directories[index].size == 0)) throw std::runtime_error("PE data directory is incomplete");
    }
    auto table = optionalHeader + optionalSize;
    if (table > headerSize || static_cast<std::size_t>(count) * 40 > headerSize - table) throw std::runtime_error("PE section table exceeds its headers");
    for (std::size_t index = 0; index < count; ++index) {
        auto offset = table + index * 40;
        Section section{Read<std::uint32_t>(data, offset + 12), Read<std::uint32_t>(data, offset + 8), Read<std::uint32_t>(data, offset + 20), Read<std::uint32_t>(data, offset + 16), Read<std::uint32_t>(data, offset + 36)};
        if (section.offset > data.size() || section.size > data.size() - section.offset || static_cast<std::uint64_t>(section.address) + std::max(section.size, section.virtualSize) > 0x100000000ULL) throw std::runtime_error("PE section exceeds its file or address bounds");
        sections.push_back(section);
    }
    std::ranges::sort(sections, {}, &Section::address);
    for (std::size_t index = 1; index < sections.size(); ++index) {
        const auto& before = sections[index - 1];
        if (static_cast<std::uint64_t>(before.address) + std::max(before.virtualSize, before.size) > sections[index].address) throw std::runtime_error("PE virtual sections overlap");
    }
    if (entry != 0 && !IsExecutable(entry)) throw std::runtime_error("PE entry point is not executable");
}

std::size_t AnyBox::Relinker::Image::FileOffset(std::uint32_t address, std::size_t size) const {
    if (address < headerSize && size <= headerSize - address) return address;
    for (const auto& section : sections) {
        if (address < section.address) continue;
        auto offset = static_cast<std::uint64_t>(address) - section.address;
        auto available = section.virtualSize == 0 ? section.size : std::min(section.virtualSize, section.size);
        if (offset <= available && size <= available - offset) return section.offset + static_cast<std::size_t>(offset);
    }
    throw std::runtime_error("PE address is not backed by file data");
}

bool AnyBox::Relinker::Image::IsExecutable(std::uint32_t address) const noexcept {
    for (const auto& section : sections) {
        auto available = section.virtualSize == 0 ? section.size : std::min(section.virtualSize, section.size);
        if ((section.flags & 0x20000000) != 0 && address >= section.address && address - section.address < available) return true;
    }
    return false;
}

std::span<const std::byte> AnyBox::Relinker::Image::Code(std::uint32_t address) const {
    for (const auto& section : sections) {
        auto available = section.virtualSize == 0 ? section.size : std::min(section.virtualSize, section.size);
        if ((section.flags & 0x20000000) == 0 || address < section.address || address - section.address >= available) continue;
        auto offset = address - section.address;
        return GetBytes().subspan(section.offset + offset, available - offset);
    }
    throw std::runtime_error("PE instruction address is not executable");
}

void AnyBox::Relinker::Image::Finalize() {
    std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(optionalHeader + 64), 4, std::byte{});
    if (directoryCount > 4) {
        std::fill_n(bytes.begin() + static_cast<std::ptrdiff_t>(optionalHeader + 112 + 4 * 8), 8, std::byte{});
        directories[4] = {};
    }
}
