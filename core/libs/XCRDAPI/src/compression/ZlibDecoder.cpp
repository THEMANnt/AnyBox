#include "ZlibDecoder.hpp"
#include "../metrics/Memory.hpp"

#include <libdeflate.h>

#include <new>
#include <stdexcept>

void AnyBox::Storage::ReleaseZlibDecoder::operator()(libdeflate_decompressor* decoder) const noexcept {
    libdeflate_free_decompressor(decoder);
}

AnyBox::Storage::ZlibDecoder::ZlibDecoder() {
    libdeflate_options options{};
    options.sizeof_options = sizeof(options);
    options.malloc_func = AllocateCodecMemory;
    options.free_func = FreeCodecMemory;
    decoder.reset(libdeflate_alloc_decompressor_ex(&options));
    if (!decoder) throw std::bad_alloc();
}

void AnyBox::Storage::ZlibDecoder::Decode(std::span<const std::byte> input, std::span<std::byte> output) {
    if (input.empty()) throw std::invalid_argument("Zlib source is empty");
    std::size_t consumed = 0;
    std::size_t written = 0;
    auto result = libdeflate_zlib_decompress_ex(decoder.get(), input.data(), input.size(), output.data(), output.size(), &consumed, &written);

    if (result != LIBDEFLATE_SUCCESS) throw std::runtime_error("Invalid RFC 1950 zlib stream or insufficient output capacity");
    if (consumed != input.size()) throw std::runtime_error("Zlib source contains data after the compressed stream");
    if (written != output.size()) throw std::runtime_error("Zlib output size does not match the DirectStorage request");
}
