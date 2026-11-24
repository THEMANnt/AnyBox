#include "XmaContext.hpp"
#include "../memory/Heap.hpp"
#include <anybox/Unimplemented.hpp>
#include <array>
#include <stdexcept>

namespace {
constexpr std::array<std::uint32_t, 4> SampleRates{24000, 32000, 44100, 48000};
}

AnyBox::AcpHal::XmaContext::XmaContext(std::span<std::uint32_t, 16> words, std::uint32_t index, std::uint64_t& sequence, std::optional<Codecs::XmaFrameDecoder> previous) : words(words), channels(((words[1] >> 29) & 1) + 1), rateIndex((words[1] >> 27) & 3), index(index), sequence(sequence), decoder(std::move(previous)) {
    validate();
    GetHeap().GetMemory(words[8], 512);
    if (decoder) publishState();
    else decoder.emplace(SampleRates[rateIndex], channels);
}

std::uint32_t AnyBox::AcpHal::XmaContext::get(std::size_t word, std::uint32_t shift, std::uint32_t mask) const { return (words[word] >> shift) & mask; }

void AnyBox::AcpHal::XmaContext::set(std::size_t word, std::uint32_t shift, std::uint32_t mask, std::uint32_t value) {
    words[word] = (words[word] & ~(mask << shift)) | ((value & mask) << shift);
}

void AnyBox::AcpHal::XmaContext::validate() const {
    if (get(1, 29, 1) + 1 != channels || get(1, 27, 3) != rateIndex) throw std::logic_error("Active XMA stream format changed");
    if (get(1, 14, 7) != 0) ANYBOX_NOT_IMPLEMENTED();
    if (get(1, 20, 15) != 4 || get(1, 30, 1) != 0 || get(9, 30, 3) != 0) ANYBOX_NOT_IMPLEMENTED();
    if (get(2, 26, 63) != 0 || get(3, 26, 63) != 0) throw std::runtime_error("XMA context contains a decoder error");
    for (std::size_t word = 10; word < words.size(); ++word) if (words[word] != 0) ANYBOX_NOT_IMPLEMENTED();
    auto capacity = get(0, 22, 31);
    if (capacity == 0 || get(0, 27, 31) >= capacity || get(9, 0, 31) >= capacity) throw std::invalid_argument("Invalid XMA output ring offsets");
}

std::uint32_t AnyBox::AcpHal::XmaContext::availableBlocks() const {
    auto capacity = get(0, 22, 31);
    auto write = get(0, 27, 31);
    auto read = get(9, 0, 31);
    if (write == read) return get(1, 31, 1) != 0 ? capacity : 0;
    return read > write ? read - write : capacity - write + read;
}

void AnyBox::AcpHal::XmaContext::Consume(std::uint32_t blocks) {
    validate();
    auto capacity = get(0, 22, 31);
    if (blocks > capacity - availableBlocks()) throw std::out_of_range("XMA output consumption exceeds produced data");
    set(9, 0, 31, (get(9, 0, 31) + blocks) % capacity);
    if (blocks != 0) set(1, 31, 1, 1);
    Process();
}
