#ifndef ANYBOX_STORAGE_PHYSICAL_TESTS_HPP
#define ANYBOX_STORAGE_PHYSICAL_TESTS_HPP

#include "../compression/Tests.hpp"

#include <xmem.hpp>
#include <anybox/PhysicalMemory.hpp>

namespace AnyBox::Tests {

class PhysicalBuffer {
public:
    explicit PhysicalBuffer(std::size_t count);
    PhysicalBuffer(const PhysicalBuffer&) = delete;
    PhysicalBuffer& operator=(const PhysicalBuffer&) = delete;
    ~PhysicalBuffer();
    std::span<std::byte> GetMemory() const;
    std::span<const std::uint64_t> GetFrames() const;
    void Map(std::span<const std::uint64_t> order);
    void Unmap();
    void FreePages();

private:
    void releaseWindow();
    std::vector<std::uint64_t> frames;
    std::byte* memory = nullptr;
    bool mapped = false;
    bool allocated = false;
};

struct PhysicalPacket {
    std::vector<std::byte> plain;
    std::vector<std::byte> packed;
    PhysicalPacket();
};

void CheckPhysicalCopies();
void CheckPhysicalMemoryDecode();
void CheckPhysicalFileRead();
void CheckPhysicalOverlaps();
void CheckDisjointSharedPage();
void CheckPhysicalLifetime();
void CheckPhysicalCancellation();
void CheckPhysicalFailure();
void CheckRequestPadding();

}

#endif
