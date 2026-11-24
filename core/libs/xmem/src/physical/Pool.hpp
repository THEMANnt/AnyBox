#ifndef ANYBOX_XMEM_PHYSICAL_POOL_HPP
#define ANYBOX_XMEM_PHYSICAL_POOL_HPP

#include <anybox/platform/Windows.hpp>
#include <anybox/PhysicalMemory.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <vector>

namespace AnyBox::XMem {

constexpr std::size_t PhysicalPageSize = AnyBox::PhysicalPageSize;

struct Section {
    HANDLE handle;
    std::byte* memory;
    Section(HANDLE handle, std::size_t size);
    ~Section();
    Section(const Section&) = delete;
    Section& operator=(const Section&) = delete;
};

struct Page {
    std::shared_ptr<Section> section;
    std::uint64_t offset;
    std::uint64_t frame;
};

struct MappingChange {
    void* address;
    std::optional<Page> previous;
    std::optional<Page> next;
};

class Pool {
public:
    void Allocate(std::uint64_t flags, std::size_t count, std::uint64_t* frames);
    void Free(std::span<const std::uint64_t> frames);
    IPhysicalPageLease* Acquire(std::span<const std::uint64_t> frames);
    IPhysicalPageLease* AcquireMapped(std::uintptr_t address, std::size_t count);
    void Release(std::span<const Page> retained, std::uintptr_t mappingBase) noexcept;
    void Map(std::span<void* const> addresses, const std::uint64_t* frames);

private:
    IPhysicalPageLease* retain(std::vector<Page> retained, std::uintptr_t mappingBase);
    std::mutex mutex;
    std::uint64_t nextFrame = 16;
    std::map<std::uint64_t, Page> pages;
    std::map<std::uintptr_t, Page> mappings;
    std::map<std::uint64_t, std::size_t> pinned;
    std::map<std::uintptr_t, std::size_t> mappedPins;
};

Pool& GetPool();
void EnsurePlaceholder(void* address);
void ApplyMapping(const MappingChange& change);
void RestoreMapping(const MappingChange& change);

}

#endif
