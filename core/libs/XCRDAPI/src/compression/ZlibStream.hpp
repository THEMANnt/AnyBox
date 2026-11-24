#ifndef ANYBOX_STORAGE_ZLIB_STREAM_HPP
#define ANYBOX_STORAGE_ZLIB_STREAM_HPP

#include <zlib.h>

#include <cstddef>
#include <span>

namespace AnyBox::Storage {

class ZlibStream {
public:
    ZlibStream();
    ZlibStream(const ZlibStream&) = delete;
    ZlibStream& operator=(const ZlibStream&) = delete;
    ~ZlibStream();
    using OutputBuffer = std::span<std::byte> (*)(void*, std::size_t);
    void Reset(std::span<std::byte> destination);
    void Reset(std::size_t size, void* context, OutputBuffer output);
    void Process(std::span<const std::byte> input, bool finalChunk);

private:
    void provideOutput();
    z_stream stream{};
    void* outputContext = nullptr;
    OutputBuffer outputBuffer = nullptr;
    std::size_t expectedSize = 0;
    unsigned char overflow = 0;
    bool overflowActive = false;
    bool finished = false;
};

}

#endif
