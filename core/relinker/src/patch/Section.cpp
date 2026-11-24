#include "Helpers.hpp"
#include <algorithm>

void AnyBox::Relinker::Image::AppendLowerings(std::span<const LoweringSite> sites) {
    if (sites.empty()) return;
    if (sections.size() >= 96 || directoryCount < 4) throw std::runtime_error("PE header cannot describe another executable section and its exception directory");
    auto optionalSize = Read<std::uint16_t>(GetBytes(), optionalHeader - 4);
    auto header = optionalHeader + optionalSize + sections.size() * 40;
    if (header > headerSize || 40 > headerSize - header) throw std::runtime_error("PE header has no space for a lowering section");
    if (!std::ranges::all_of(GetBytes().subspan(header, 40), [](auto value) { return value == std::byte{}; })) throw std::runtime_error("PE lowering section header would overwrite existing header data");
    auto sectionAlignment = Read<std::uint32_t>(GetBytes(), optionalHeader + 32);
    auto fileAlignment = Read<std::uint32_t>(GetBytes(), optionalHeader + 36);
    if (sectionAlignment < 4096 || fileAlignment < 512 || fileAlignment > 65536 || fileAlignment > sectionAlignment) throw std::runtime_error("Unsupported PE section or file alignment for instruction lowering");
    auto address = static_cast<std::uint64_t>(Read<std::uint32_t>(GetBytes(), optionalHeader + 56));
    for (const auto& section : sections) address = std::max(address, static_cast<std::uint64_t>(section.address) + std::max(section.virtualSize, section.size));
    auto sectionAddress = Align(address, sectionAlignment);
    auto rawOffset = Align(bytes.size(), fileAlignment);
    auto directory = directories[3];
    auto functions = directory.size == 0 ? std::span<const std::byte>{} : GetBytes().subspan(FileOffset(directory.address, directory.size), directory.size);
    auto content = BuildHelpers(sites, functions, sectionAddress);
    auto rawSize = Align(content.bytes.size(), fileAlignment);
    auto imageSize = Align(static_cast<std::uint64_t>(sectionAddress) + content.bytes.size(), sectionAlignment);
    auto fileSize = CheckedRva(static_cast<std::uint64_t>(rawOffset) + rawSize);
    std::vector<std::pair<std::size_t, std::size_t>> intervals;
    Decoder decoder;
    for (const auto& site : sites) {
        if (!IsExecutable(site.address) || decoder.Decode(Code(site.address)).native.length != site.length) throw std::runtime_error("PE lowering call site is not a complete executable instruction");
        auto offset = FileOffset(site.address, site.length);
        intervals.emplace_back(offset, offset + site.length);
    }
    std::ranges::sort(intervals);
    for (std::size_t index = 1; index < intervals.size(); ++index) {
        if (intervals[index - 1].second > intervals[index].first) throw std::runtime_error("PE lowering call sites overlap");
    }
    std::vector<std::byte> output;
    output.reserve(fileSize);
    output.insert(output.end(), bytes.begin(), bytes.end());
    output.resize(fileSize);
    std::ranges::copy(content.bytes, output.begin() + rawOffset);
    for (const auto& call : content.calls) {
        auto offset = FileOffset(call[0], call[1]);
        output[offset] = std::byte{0xE8};
        Write<std::uint32_t>(output, offset + 1, call[2]);
        std::fill_n(output.begin() + static_cast<std::ptrdiff_t>(offset + 5), call[1] - 5, std::byte{0x90});
    }
    constexpr std::array<char, 8> name{'.', 'a', 'b', 'x', 'c', 'o', 'd', 'e'};
    std::memcpy(output.data() + header, name.data(), name.size());
    Write<std::uint32_t>(output, header + 8, CheckedRva(content.bytes.size()));
    Write<std::uint32_t>(output, header + 12, sectionAddress);
    Write<std::uint32_t>(output, header + 16, rawSize);
    Write<std::uint32_t>(output, header + 20, rawOffset);
    Write<std::uint32_t>(output, header + 36, 0x60000060);
    Write<std::uint16_t>(output, optionalHeader - 18, static_cast<std::uint16_t>(sections.size() + 1));
    Write<std::uint32_t>(output, optionalHeader + 4, CheckedRva(static_cast<std::uint64_t>(Read<std::uint32_t>(GetBytes(), optionalHeader + 4)) + rawSize));
    Write<std::uint32_t>(output, optionalHeader + 8, CheckedRva(static_cast<std::uint64_t>(Read<std::uint32_t>(GetBytes(), optionalHeader + 8)) + rawSize));
    Write<std::uint32_t>(output, optionalHeader + 56, imageSize);
    Write<std::uint32_t>(output, optionalHeader + 136, content.exceptions.address);
    Write<std::uint32_t>(output, optionalHeader + 140, content.exceptions.size);
    Image rewritten(std::move(output));
    rewritten.Finalize();
    *this = std::move(rewritten);
}
