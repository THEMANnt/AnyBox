#include "Tests.hpp"
#include "../../src/compression/ZlibDecoder.hpp"

#include <algorithm>

void AnyBox::Tests::CheckZlibDecoder() {
    CompressedData data;
    Storage::ZlibDecoder decoder;
    std::vector<std::byte> output(data.plain.size() + 2, std::byte{0x71});
    auto destination = std::span(output).subspan(1, data.plain.size());
    decoder.Decode(data.packed, destination);
    Require(std::equal(data.plain.begin(), data.plain.end(), destination.begin()), "Zlib decoding changed payload bytes");
    Require(output.front() == std::byte{0x71} && output.back() == std::byte{0x71}, "Zlib decoding overran its output");

    auto truncated = std::span(data.packed).first(data.packed.size() - 1);
    ExpectException<std::runtime_error>([&] { decoder.Decode(truncated, destination); });
    data.packed.back() ^= std::byte{1};
    ExpectException<std::runtime_error>([&] { decoder.Decode(data.packed, destination); });
    data.packed.back() ^= std::byte{1};
    data.packed.push_back(std::byte{0});
    ExpectException<std::runtime_error>([&] { decoder.Decode(data.packed, destination); });
    data.packed.pop_back();
    ExpectException<std::runtime_error>([&] { decoder.Decode(data.packed, destination.first(destination.size() - 1)); });
    ExpectException<std::runtime_error>([&] { decoder.Decode(data.packed, output); });
    ExpectException<std::invalid_argument>([&] { decoder.Decode({}, destination); });

    std::unique_ptr<libdeflate_compressor, decltype(&libdeflate_free_compressor)> compressor(libdeflate_alloc_compressor(6), libdeflate_free_compressor);
    Require(compressor != nullptr, "Raw DEFLATE test compressor allocation failed");
    std::vector<std::byte> raw(libdeflate_deflate_compress_bound(compressor.get(), data.plain.size()));
    auto rawSize = libdeflate_deflate_compress(compressor.get(), data.plain.data(), data.plain.size(), raw.data(), raw.size());
    Require(rawSize != 0, "Raw DEFLATE test compression failed");
    raw.resize(rawSize);
    ExpectException<std::runtime_error>([&] { decoder.Decode(raw, destination); });
    decoder.Decode(data.packed, destination);
    Require(std::equal(data.plain.begin(), data.plain.end(), destination.begin()), "Zlib decoder failed after rejecting invalid data");
}
