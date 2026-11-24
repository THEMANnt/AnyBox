#ifndef ANYBOX_STORAGE_ZLIB_DECODER_HPP
#define ANYBOX_STORAGE_ZLIB_DECODER_HPP

#include <cstddef>
#include <memory>
#include <span>

struct libdeflate_decompressor;

namespace AnyBox::Storage {

struct ReleaseZlibDecoder {
    void operator()(libdeflate_decompressor* decoder) const noexcept;
};

class ZlibDecoder {
public:
    ZlibDecoder();
    void Decode(std::span<const std::byte> input, std::span<std::byte> output);

private:
    std::unique_ptr<libdeflate_decompressor, ReleaseZlibDecoder> decoder;
};

}

#endif
