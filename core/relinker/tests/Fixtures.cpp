#include "Fixtures.hpp"

std::vector<std::byte> AnyBox::Relinker::Tests::Fixture(std::span<const std::uint8_t> code) {
    std::vector<std::byte> bytes(1536);
    Put<std::uint16_t>(bytes, 0, 0x5A4D);
    Put<std::uint32_t>(bytes, 0x3C, 0x80);
    Put<std::uint32_t>(bytes, 0x80, 0x4550);
    Put<std::uint16_t>(bytes, 0x84, 0x8664);
    Put<std::uint16_t>(bytes, 0x86, 2);
    Put<std::uint16_t>(bytes, 0x94, 240);
    Put<std::uint16_t>(bytes, 0x98, 0x20B);
    Put<std::uint32_t>(bytes, 0xA8, 0x1000);
    Put<std::uint64_t>(bytes, 0xB0, 0x140000000ULL);
    Put<std::uint32_t>(bytes, 0xB8, 4096);
    Put<std::uint32_t>(bytes, 0xBC, 512);
    Put<std::uint32_t>(bytes, 0xD0, 12288);
    Put<std::uint32_t>(bytes, 0xD4, 512);
    Put<std::uint32_t>(bytes, 0x104, 16);
    Put<std::uint32_t>(bytes, 0x120, 0x2000);
    Put<std::uint32_t>(bytes, 0x124, 12);
    Put<std::uint32_t>(bytes, 0x190, 64);
    Put<std::uint32_t>(bytes, 0x194, 0x1000);
    Put<std::uint32_t>(bytes, 0x198, 512);
    Put<std::uint32_t>(bytes, 0x19C, 512);
    Put<std::uint32_t>(bytes, 0x1AC, 0x60000020);
    Put<std::uint32_t>(bytes, 0x1B8, 64);
    Put<std::uint32_t>(bytes, 0x1BC, 0x2000);
    Put<std::uint32_t>(bytes, 0x1C0, 512);
    Put<std::uint32_t>(bytes, 0x1C4, 1024);
    Put<std::uint32_t>(bytes, 0x1D4, 0x40000040);
    Put<std::uint32_t>(bytes, 1024, 0x1000);
    Put<std::uint32_t>(bytes, 1028, 0x1000 + static_cast<std::uint32_t>(code.size()));
    Put<std::uint32_t>(bytes, 1032, 0x2030);
    std::memcpy(bytes.data() + 512, code.data(), code.size());
    return bytes;
}
