#include <anybox/relinker/Image.hpp>
#include <unordered_set>

std::uint64_t AnyBox::Relinker::Image::GetStackAllocation(std::uint32_t address) const {
    auto directory = directories[3];
    if (directory.size == 0) return 0;
    if (directory.size % 12 != 0) throw std::runtime_error("PE runtime function table has an invalid size");
    auto table = FileOffset(directory.address, directory.size);
    std::size_t first = 0, last = directory.size / 12;
    while (first < last) {
        auto middle = first + (last - first) / 2;
        if (Read<std::uint32_t>(GetBytes(), table + middle * 12) <= address) first = middle + 1;
        else last = middle;
    }
    if (first == 0) return 0;
    auto function = table + (first - 1) * 12;
    auto begin = Read<std::uint32_t>(GetBytes(), function);
    if (address >= Read<std::uint32_t>(GetBytes(), function + 4)) return 0;
    auto unwindAddress = Read<std::uint32_t>(GetBytes(), function + 8);
    auto position = address - begin;
    std::unordered_set<std::uint32_t> seen;
    std::uint64_t allocation = 0;
    for (;;) {
        if (!seen.insert(unwindAddress).second || seen.size() > 32) throw std::runtime_error("PE chained unwind information cycles or exceeds its limit");
        auto unwind = FileOffset(unwindAddress, 4);
        auto flags = Read<std::uint8_t>(GetBytes(), unwind);
        if ((flags & 7) != 1) throw std::runtime_error("Unsupported PE unwind information version for instruction lowering");
        auto count = Read<std::uint8_t>(GetBytes(), unwind + 2);
        FileOffset(unwindAddress, 4 + static_cast<std::size_t>(count) * 2);
        for (std::size_t index = 0; index < count;) {
            auto code = unwind + 4 + index * 2;
            auto operation = Read<std::uint8_t>(GetBytes(), code + 1);
            auto info = operation >> 4;
            std::size_t slots = 1;
            std::uint64_t size = 0;
            switch (operation & 15) {
            case 0: size = 8; break;
            case 1:
                slots = info == 0 ? 2 : 3;
                if (info > 1) throw std::runtime_error("Invalid PE large stack allocation unwind code");
                break;
            case 2: size = static_cast<unsigned>(info) * 8 + 8; break;
            case 3: break;
            case 4: case 8: slots = 2; break;
            case 5: case 9: slots = 3; break;
            case 10:
                if (info > 1) throw std::runtime_error("Invalid PE machine frame unwind code");
                size = info == 0 ? 40 : 48;
                break;
            default: throw std::runtime_error("Unsupported PE unwind operation for instruction lowering");
            }
            if (slots > count - index) throw std::runtime_error("PE unwind operation exceeds its code array");
            if ((operation & 15) == 1) size = info == 0 ? static_cast<std::uint64_t>(Read<std::uint16_t>(GetBytes(), code + 2)) * 8 : Read<std::uint32_t>(GetBytes(), code + 2);
            if (Read<std::uint8_t>(GetBytes(), code) <= position) allocation += size;
            index += slots;
        }
        if ((flags & 0x20) == 0) return allocation;
        auto chain = FileOffset(unwindAddress, 4 + ((static_cast<std::size_t>(count) + 1) & ~std::size_t{1}) * 2 + 12);
        unwindAddress = Read<std::uint32_t>(GetBytes(), chain + 4 + ((static_cast<std::size_t>(count) + 1) & ~std::size_t{1}) * 2 + 8);
        position = UINT32_MAX;
    }
}
