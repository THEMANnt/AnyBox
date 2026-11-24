#include "Queue.hpp"
#include "../metrics/Requests.hpp"

#include <algorithm>
#include <cstring>
#include <optional>

std::span<std::byte> AnyBox::Storage::Queue::captureSource(const MemoryView& source) {
    if (source.GetSize() > overlapCapacity) {
        auto replacement = MakeTrackedArray<std::byte>(source.GetSize());
        overlapBuffer = std::move(replacement);
        overlapCapacity = source.GetSize();
    }
    for (std::size_t position = 0; position < source.GetSize();) {
        auto input = source.GetChunk(position);
        std::memcpy(overlapBuffer.get() + position, input.data(), input.size());
        position += input.size();
    }
    return {overlapBuffer.get(), source.GetSize()};
}

void AnyBox::Storage::Queue::readMemory(ReadOperation& operation) {
    const auto& request = operation.request;
    auto* source = &operation.source;
    auto& destination = operation.destination;
    bool compressed = (request.options & 1) != 0;
    auto input = source->GetContiguousMemory();
    auto output = destination.GetContiguousMemory();
    bool snapshot = compressed || input.empty() || output.empty() || source->IsPhysical() != destination.IsPhysical();
    std::optional<MemoryView> retained;
    if (snapshot && source->MayOverlap(destination)) {
        auto captured = captureSource(*source);
        retained.emplace(captured.data(), captured.size(), false);
        source = &*retained;
        input = source->GetContiguousMemory();
    }
    if (!compressed) {
        for (std::size_t position = 0; position < source->GetSize();) {
            auto from = source->GetChunk(position);
            auto to = destination.GetChunk(position);
            auto chunkBytes = std::min(from.size(), to.size());
            std::memmove(to.data(), from.data(), chunkBytes);
            position += chunkBytes;
        }
        return;
    }
    if (!input.empty() && !output.empty()) {
        decompress(input, output);
        return;
    }

    WorkStatistics statistics(WorkType::Decompression);
    startStream(destination);
    for (std::size_t position = 0; position < source->GetSize();) {
        auto chunk = source->GetChunk(position);
        position += chunk.size();
        processStream(chunk, position == source->GetSize());
    }
}
