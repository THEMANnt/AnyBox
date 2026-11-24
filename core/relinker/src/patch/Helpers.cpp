#include "Helpers.hpp"
#include <algorithm>

AnyBox::Relinker::HelperSection AnyBox::Relinker::BuildHelpers(std::span<const LoweringSite> sites, std::span<const std::byte> existing, std::uint32_t sectionAddress) {
    if (existing.size() % 12 != 0) throw std::runtime_error("PE runtime function table size is invalid");
    std::vector<std::array<std::uint32_t, 3>> functions;
    for (std::size_t index = 0; index < existing.size(); index += 12) functions.push_back({Read<std::uint32_t>(existing, index), Read<std::uint32_t>(existing, index + 4), Read<std::uint32_t>(existing, index + 8)});
    HelperSection output;
    for (const auto& site : sites) {
        const auto& helper = site.function;
        if (helper.code.empty() || helper.ranges.empty() || site.length < 5 || site.length > ZYDIS_MAX_INSTRUCTION_LENGTH) throw std::runtime_error("PE lowering helper or call site is invalid");
        output.bytes.resize(Align(output.bytes.size(), 16), std::byte{0xCC});
        auto helperAddress = CheckedRva(sectionAddress + static_cast<std::uint64_t>(output.bytes.size()));
        auto displacement = static_cast<std::int64_t>(helperAddress) - site.address - 5;
        if (displacement < INT32_MIN || displacement > INT32_MAX) throw std::runtime_error("PE lowering helper is outside the relative call range");
        output.calls.push_back({site.address, site.length, static_cast<std::uint32_t>(static_cast<std::int32_t>(displacement))});
        output.bytes.insert(output.bytes.end(), helper.code.begin(), helper.code.end());
        std::uint32_t previous = 0;
        for (const auto& range : helper.ranges) {
            if (range.begin != previous || range.begin >= range.end || range.end > helper.code.size() || range.prologSize > range.end - range.begin) throw std::runtime_error("PE lowering unwind ranges do not cover its helper code");
            previous = range.end;
            output.bytes.resize(Align(output.bytes.size(), 4));
            auto offset = output.bytes.size();
            auto unwindAddress = CheckedRva(sectionAddress + static_cast<std::uint64_t>(offset));
            if (range.stackSize == 0) {
                if (range.prologSize != 0) throw std::runtime_error("PE leaf helper has a stack prolog");
                output.bytes.insert(output.bytes.end(), {std::byte{1}, std::byte{}, std::byte{}, std::byte{}});
            } else {
                if (range.stackSize < 8 || range.stackSize > 128 || range.stackSize % 8 != 0 || range.prologSize == 0) throw std::runtime_error("PE lowering helper stack frame cannot be encoded");
                auto operation = static_cast<std::byte>(((range.stackSize - 8) / 8) << 4 | 2);
                output.bytes.insert(output.bytes.end(), {std::byte{1}, static_cast<std::byte>(range.prologSize), std::byte{1}, std::byte{}, static_cast<std::byte>(range.prologSize), operation, std::byte{}, std::byte{}});
            }
            functions.push_back({CheckedRva(static_cast<std::uint64_t>(helperAddress) + range.begin), CheckedRva(static_cast<std::uint64_t>(helperAddress) + range.end), unwindAddress});
        }
        if (previous != helper.code.size()) throw std::runtime_error("PE lowering helper has code outside its unwind ranges");
    }
    std::ranges::sort(functions);
    for (std::size_t index = 0; index < functions.size(); ++index) {
        if (functions[index][0] >= functions[index][1] || (index != 0 && functions[index - 1][1] > functions[index][0])) throw std::runtime_error("PE runtime function ranges are invalid or overlap");
    }
    output.bytes.resize(Align(output.bytes.size(), 4));
    output.exceptions = {CheckedRva(sectionAddress + static_cast<std::uint64_t>(output.bytes.size())), CheckedRva(functions.size() * 12ULL)};
    for (const auto& function : functions) {
        for (auto value : function) {
            auto encoded = std::as_bytes(std::span(&value, 1));
            output.bytes.insert(output.bytes.end(), encoded.begin(), encoded.end());
        }
    }
    return output;
}
