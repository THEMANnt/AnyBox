#pragma once

#include "Binary.hpp"
#include "Lowering.hpp"
#include <array>
#include <cstdint>
#include <vector>
#include <unordered_set>

namespace AnyBox::Relinker {
struct Section {
    std::uint32_t address;
    std::uint32_t virtualSize;
    std::uint32_t offset;
    std::uint32_t size;
    std::uint32_t flags;
};
struct Directory { std::uint32_t address = 0; std::uint32_t size = 0; };
class Image {
public:
    explicit Image(std::vector<std::byte> bytes);
    std::span<const std::byte> GetBytes() const noexcept { return bytes; }
    std::span<std::byte> GetBytes() noexcept { return bytes; }
    std::span<const Section> GetSections() const noexcept { return sections; }
    std::uint64_t GetBase() const noexcept { return base; }
    std::size_t FileOffset(std::uint32_t address, std::size_t size) const;
    bool IsExecutable(std::uint32_t address) const noexcept;
    std::span<const std::byte> Code(std::uint32_t address) const;
    std::vector<std::uint32_t> EntryPoints() const;
    std::unordered_set<std::uint32_t> ImportSlots() const;
    bool IsReadOnly(std::uint32_t address, std::size_t size) const noexcept;
    std::uint64_t GetStackAllocation(std::uint32_t address) const;
    void AppendLowerings(std::span<const LoweringSite> sites);
    void Finalize();

private:
    void appendExports(std::vector<std::uint32_t>& entries) const;
    void appendRelocations(std::vector<std::uint32_t>& entries) const;
    void appendLoaderEntries(std::vector<std::uint32_t>& entries) const;
    void appendGuardTable(std::vector<std::uint32_t>& entries, std::size_t offset, std::size_t stride) const;
    std::vector<std::byte> bytes;
    std::vector<Section> sections;
    std::array<Directory, 16> directories{};
    std::uint64_t base = 0;
    std::uint32_t entry = 0;
    std::uint32_t headerSize = 0;
    std::size_t optionalHeader = 0;
    std::uint32_t directoryCount = 0;
};
}
