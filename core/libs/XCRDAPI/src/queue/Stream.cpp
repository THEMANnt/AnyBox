#include "Queue.hpp"

#include <stdexcept>
#include <system_error>

namespace {

std::span<std::byte> GetOutput(void* context, std::size_t position) {
    return static_cast<AnyBox::Storage::MemoryView*>(context)->GetChunk(position);
}

}

void AnyBox::Storage::Queue::startStream(MemoryView& destination) {
    if (!streamDecoder) streamDecoder.emplace();
    auto contiguous = destination.GetContiguousMemory();
    if (!contiguous.empty()) streamDecoder->Reset(contiguous);
    else streamDecoder->Reset(destination.GetSize(), &destination, GetOutput);
}

void AnyBox::Storage::Queue::processStream(std::span<const std::byte> source, bool finalChunk) {
    try {
        streamDecoder->Process(source, finalChunk);
    } catch (const std::runtime_error& error) {
        throw std::system_error(ERROR_INVALID_DATA, std::system_category(), error.what());
    } catch (const std::length_error& error) {
        throw std::system_error(ERROR_INVALID_DATA, std::system_category(), error.what());
    }
}
