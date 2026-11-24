#include <anybox/relinker/Image.hpp>
#include <algorithm>

std::unordered_set<std::uint32_t> AnyBox::Relinker::Image::ImportSlots() const {
    std::unordered_set<std::uint32_t> slots;
    auto append = [&](std::uint32_t table, std::uint32_t lookup) {
        if (table == 0) throw std::runtime_error("PE import descriptor has no address table");
        for (std::uint64_t index = 0;; ++index) {
            auto address = static_cast<std::uint64_t>(table) + index * 8;
            auto source = static_cast<std::uint64_t>(lookup == 0 ? table : lookup) + index * 8;
            if (address > UINT32_MAX || source > UINT32_MAX) throw std::runtime_error("PE import thunk address overflows");
            auto value = Read<std::uint64_t>(GetBytes(), FileOffset(static_cast<std::uint32_t>(source), 8));
            FileOffset(static_cast<std::uint32_t>(address), 8);
            if (value == 0) break;
            slots.insert(static_cast<std::uint32_t>(address));
        }
    };
    auto walk = [&](Directory directory, std::size_t stride, bool delayed) {
        if (directory.size == 0) return;
        auto offset = FileOffset(directory.address, directory.size);
        for (std::size_t index = 0; index + stride <= directory.size; index += stride) {
            auto descriptor = GetBytes().subspan(offset + index, stride);
            bool empty = true;
            for (auto value : descriptor) empty &= value == std::byte{};
            if (empty) return;
            if (!delayed) {
                append(Read<std::uint32_t>(descriptor, 16), Read<std::uint32_t>(descriptor, 0));
                continue;
            }
            auto attributes = Read<std::uint32_t>(descriptor, 0);
            if ((attributes & ~1U) != 0) throw std::runtime_error("PE delay import attributes are invalid");
            auto address = [&](std::size_t field) {
                auto value = Read<std::uint32_t>(descriptor, field);
                if (value == 0 || (attributes & 1) != 0) return value;
                if (value < base) throw std::runtime_error("PE delay import address lies outside the image");
                return static_cast<std::uint32_t>(value - base);
            };
            append(address(12), address(16));
        }
        throw std::runtime_error("PE import descriptor array has no terminator");
    };
    walk(directories[1], 20, false);
    walk(directories[13], 32, true);
    return slots;
}

bool AnyBox::Relinker::Image::IsReadOnly(std::uint32_t address, std::size_t size) const noexcept {
    for (const auto& section : sections) {
        auto available = section.virtualSize == 0 ? section.size : std::min(section.virtualSize, section.size);
        if ((section.flags & 0x80000000) != 0 || address < section.address) continue;
        auto offset = static_cast<std::uint64_t>(address) - section.address;
        if (offset <= available && size <= available - offset) return true;
    }
    return false;
}
