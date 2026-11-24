#include <anybox/graphics/container/Native.hpp>
#include "Integer.hpp"
#include <libdeflate.h>
#include <memory>

namespace {
struct DecompressorDeleter {
    void operator()(libdeflate_decompressor* value) const noexcept { libdeflate_free_decompressor(value); }
};
}

std::vector<std::byte> AnyBox::Graphics::Container::UnpackXbch(std::span<const std::byte> data) {
    if (data.size() < 24 || ReadInteger(data, 0) != 0x48434258) throw std::invalid_argument("Xbox native shader compression header is invalid");
    if (ReadInteger(data, 4) != 3) throw std::runtime_error("Xbox native shader compression version is not implemented");
    if (ReadInteger(data, 12) != 0 || ReadInteger(data, 20) != 0) throw std::runtime_error("Xbox native shader compression flags are not implemented");
    auto stored = ReadInteger(data, 8);
    auto size = ReadInteger(data, 16);
    if (stored != data.size() - 24 || size == 0) throw std::invalid_argument("Xbox native shader compression sizes are invalid");
    std::unique_ptr<libdeflate_decompressor, DecompressorDeleter> decoder(libdeflate_alloc_decompressor());
    if (!decoder) throw std::bad_alloc();
    std::vector<std::byte> output(size);
    std::size_t consumed = 0;
    std::size_t written = 0;
    auto result = libdeflate_zlib_decompress_ex(decoder.get(), data.data() + 24, stored, output.data(), output.size(), &consumed, &written);
    if (result != LIBDEFLATE_SUCCESS || consumed != stored || written != size) throw std::invalid_argument("Xbox native shader compressed stream is invalid");
    FssmView verified(output);
    return output;
}
