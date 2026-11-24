#include "Tests.hpp"

AnyBox::Tests::PhysicalPacket::PhysicalPacket() : plain(100001) {
    std::uint32_t state = 0x173A8D57;
    for (std::size_t index = 0; index < plain.size(); ++index) {
        if (index % 256 >= 192) plain[index] = plain[index - 192];
        else {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            plain[index] = std::byte(state);
        }
    }
    std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(libdeflate_alloc_compressor(6), libdeflate_free_compressor);
    Require(compressor != nullptr, "Physical test compressor allocation failed");
    packed.resize(libdeflate_zlib_compress_bound(compressor.get(), plain.size()));
    auto size = libdeflate_zlib_compress(compressor.get(), plain.data(), plain.size(), packed.data(), packed.size());
    Require(size > AnyBox::PhysicalPageSize && size < plain.size(), "Physical test packet does not cross source page boundaries");
    packed.resize(size);
}
