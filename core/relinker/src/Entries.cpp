#include <anybox/relinker/Image.hpp>
#include <algorithm>

std::vector<std::uint32_t> AnyBox::Relinker::Image::EntryPoints() const {
    std::vector<std::uint32_t> entries;
    if (entry != 0) entries.push_back(entry);
    auto directory = directories[3];
    if (directory.size != 0) {
        if (directory.size % 12 != 0) throw std::runtime_error("PE runtime function directory has an invalid size");
        auto offset = FileOffset(directory.address, directory.size);
        for (std::size_t index = 0; index < directory.size; index += 12) {
            auto begin = Read<std::uint32_t>(GetBytes(), offset + index);
            auto end = Read<std::uint32_t>(GetBytes(), offset + index + 4);
            if (begin >= end || !IsExecutable(begin)) throw std::runtime_error("PE runtime function range is invalid");
            FileOffset(begin, end - begin);
            entries.push_back(begin);
        }
    }
    appendExports(entries);
    appendRelocations(entries);
    appendLoaderEntries(entries);
    std::ranges::sort(entries);
    entries.erase(std::unique(entries.begin(), entries.end()), entries.end());
    if (entries.empty()) throw std::runtime_error("PE image has no discoverable code entry points");
    return entries;
}

void AnyBox::Relinker::Image::appendExports(std::vector<std::uint32_t>& entries) const {
    auto directory = directories[0];
    if (directory.size == 0) return;
    auto offset = FileOffset(directory.address, 40);
    auto count = Read<std::uint32_t>(GetBytes(), offset + 20);
    if (count == 0) return;
    auto table = FileOffset(Read<std::uint32_t>(GetBytes(), offset + 28), static_cast<std::size_t>(count) * 4);
    for (std::size_t index = 0; index < count; ++index) {
        auto address = Read<std::uint32_t>(GetBytes(), table + index * 4);
        if (address >= directory.address && static_cast<std::uint64_t>(address) - directory.address < directory.size) continue;
        if (IsExecutable(address)) entries.push_back(address);
    }
}

void AnyBox::Relinker::Image::appendRelocations(std::vector<std::uint32_t>& entries) const {
    auto directory = directories[5];
    if (directory.size == 0) return;
    auto offset = FileOffset(directory.address, directory.size);
    for (std::size_t position = 0; position < directory.size;) {
        auto page = Read<std::uint32_t>(GetBytes(), offset + position);
        auto size = Read<std::uint32_t>(GetBytes(), offset + position + 4);
        if (size < 8 || size % 2 != 0 || size > directory.size - position) throw std::runtime_error("PE relocation block has an invalid size");
        for (std::size_t index = 8; index < size; index += 2) {
            auto value = Read<std::uint16_t>(GetBytes(), offset + position + index);
            if ((value >> 12) != 10) continue;
            auto address = static_cast<std::uint64_t>(page) + (value & 0xFFF);
            if (address > UINT32_MAX) throw std::runtime_error("PE relocation address overflows");
            auto target = Read<std::uint64_t>(GetBytes(), FileOffset(static_cast<std::uint32_t>(address), 8));
            if (target >= base && target - base <= UINT32_MAX && IsExecutable(static_cast<std::uint32_t>(target - base))) entries.push_back(static_cast<std::uint32_t>(target - base));
        }
        position += size;
    }
}
