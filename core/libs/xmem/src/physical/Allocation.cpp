#include "Pool.hpp"

#include <anybox/platform/WindowsError.hpp>

#include <exception>
#include <limits>
#include <set>
#include <stdexcept>

AnyBox::XMem::Section::Section(HANDLE handle, std::size_t size) : handle(handle), memory(static_cast<std::byte*>(MapViewOfFile(handle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, size))) {
    if (!memory) Windows::ThrowLastError("MapViewOfFile physical storage");
}

AnyBox::XMem::Section::~Section() {
    if (!UnmapViewOfFile(memory)) std::terminate();
    if (!CloseHandle(handle)) std::terminate();
}

void AnyBox::XMem::Pool::Allocate(std::uint64_t flags, std::size_t count, std::uint64_t* frames) {
    if ((flags & ~0x20400380ULL) != 0) throw std::invalid_argument("Unsupported XMem physical allocation flags");
    auto pageMode = flags & 0x20400000;
    if (pageMode == 0x400000) throw std::invalid_argument("Invalid XMem physical page mode");
    bool largePages = pageMode == 0x20000000;
    if (largePages && count % 32 != 0) throw std::invalid_argument("XMem 2 MiB physical allocation requires a multiple of 32 pages");
    if (count == 0) return;
    if (!frames) throw std::invalid_argument("XMem physical allocation output is null");
    if (count > std::numeric_limits<std::uint64_t>::max() / PhysicalPageSize) throw std::overflow_error("XMem physical allocation size overflows");
    auto size = static_cast<std::uint64_t>(count) * PhysicalPageSize;
    auto handle = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, static_cast<DWORD>(size >> 32), static_cast<DWORD>(size), nullptr);
    if (!handle) Windows::ThrowLastError("CreateFileMappingW physical pages");
    std::shared_ptr<Section> section;
    try {
        section = std::make_shared<Section>(handle, static_cast<std::size_t>(size));
    } catch (...) {
        CloseHandle(handle);
        throw;
    }
    std::lock_guard lock(mutex);
    std::uint64_t alignment = largePages ? 512 : 16;
    auto maximum = std::numeric_limits<std::uint64_t>::max();
    if (nextFrame > maximum - (alignment - 1)) throw std::overflow_error("XMem physical frame alignment overflows");
    auto firstFrame = (nextFrame + alignment - 1) & ~(alignment - 1);
    if (count > (maximum - firstFrame) / 16) throw std::overflow_error("XMem physical frame identifiers exhausted");
    std::map<std::uint64_t, Page> added;
    for (std::size_t i = 0; i < count; ++i) {
        auto frame = firstFrame + i * 16;
        added.emplace(frame, Page{section, i * PhysicalPageSize, frame});
    }
    pages.merge(added);
    for (std::size_t i = 0; i < count; ++i) frames[i] = firstFrame + i * 16;
    nextFrame = firstFrame + count * 16;
}

void AnyBox::XMem::Pool::Free(std::span<const std::uint64_t> frames) {
    std::lock_guard lock(mutex);
    std::set<std::uint64_t> requested;
    for (auto frame : frames) {
        if (!requested.insert(frame).second) throw std::invalid_argument("Duplicate XMem physical frame in free request");
        if (!pages.contains(frame)) throw std::invalid_argument("XMem physical frame is not allocated");
        if (pinned.contains(frame)) throw std::logic_error("XMem physical frame has pending memory operations");
    }
    for (const auto& [address, mapping] : mappings) {
        if (requested.contains(mapping.frame)) throw std::logic_error("XMem physical frame must be unmapped before release");
    }
    for (auto frame : frames) pages.erase(frame);
}

AnyBox::XMem::Pool& AnyBox::XMem::GetPool() {
    static Pool pool;
    return pool;
}
