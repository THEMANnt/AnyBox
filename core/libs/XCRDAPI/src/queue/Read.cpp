#include "Queue.hpp"
#include "../metrics/Requests.hpp"

#include <cstring>
#include <stdexcept>
#include <system_error>

void AnyBox::Storage::Queue::read(ReadOperation& operation) {
    const auto& request = operation.request;
    RequestStatistics statistics(description.description.priority, request.sourceSize, request.destinationSize);
    if (operation.canceled) return;
    if (request.sourceSize == 0) return;
    if (description.description.sourceType == 1) {
        readMemory(operation);
        return;
    }
    if ((request.options & 1) != 0) {
        decompressFile(operation, operation.destination);
        return;
    }
    for (std::size_t position = 0; position < request.destinationSize;) {
        auto destination = operation.destination.GetChunk(position);
        readFile(operation, destination, position);
        position += destination.size();
    }
}

void AnyBox::Storage::Queue::decompress(std::span<const std::byte> source, std::span<std::byte> destination) {
    WorkStatistics statistics(WorkType::Decompression);
    if (!decoder) decoder.emplace();
    try {
        decoder->Decode(source, destination);
    } catch (const std::runtime_error& error) {
        throw std::system_error(ERROR_INVALID_DATA, std::system_category(), error.what());
    }
}
