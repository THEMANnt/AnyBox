#ifndef ANYBOX_STORAGE_COMPRESSION_TESTS_HPP
#define ANYBOX_STORAGE_COMPRESSION_TESTS_HPP

#include "../queue/Tests.hpp"

#include <libdeflate.h>

#include <cstddef>
#include <span>
#include <vector>

namespace AnyBox::Tests {

struct CompressedData {
    std::vector<std::byte> plain;
    std::vector<std::byte> packed;

    CompressedData() : plain(16384) {
        for (std::size_t i = 0; i < plain.size(); ++i) plain[i] = std::byte((i / 16) % 251);
        std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(libdeflate_alloc_compressor(6), libdeflate_free_compressor);
        Require(compressor != nullptr, "Test compressor allocation failed");
        packed.resize(libdeflate_zlib_compress_bound(compressor.get(), plain.size()));
        auto size = libdeflate_zlib_compress(compressor.get(), plain.data(), plain.size(), packed.data(), packed.size());
        Require(size != 0, "Test zlib compression failed");
        packed.resize(size);
    }
};

void CheckTrackedMemory();
void CheckStaging();
void CheckZlibDecoder();
void CheckZlibStream();
void CheckLargeCompressedFile();
void CheckCompressedMemory();
void CheckCompressedFile();

}

#endif
