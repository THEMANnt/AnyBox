#include "Tests.hpp"
#include "../compression/TemporaryFile.hpp"

#include <algorithm>
#include <array>

void AnyBox::Tests::CheckPhysicalFileRead() {
    PhysicalPacket data;
    for (bool compressed : {false, true}) {
        CompressedFile input(compressed ? std::span(data.packed) : std::span(data.plain));
        PhysicalBuffer destination(3);
        auto frames = destination.GetFrames();
        std::array<std::uint64_t, 3> order{frames[2], frames[0], frames[1]};
        destination.Map(order);
        auto memory = destination.GetMemory();
        std::fill(memory.begin(), memory.end(), std::byte{0x57});
        QueueFixture fixture(0);
        void* object = nullptr;
        fixture.factory->OpenFile(input.Path(), Storage::FileId, &object);
        Object<IStorageFile> file(static_cast<IStorageFile*>(object));
        StorageRequest request{};
        request.options = (1ULL << 21) | static_cast<std::uint64_t>(compressed);
        request.source = file.get();
        request.sourceOffset = 16;
        request.sourceSize = static_cast<std::uint32_t>(compressed ? data.packed.size() : data.plain.size());
        request.destination = order.data();
        request.destinationPageOffset = 65503;
        request.destinationSize = static_cast<std::uint32_t>(data.plain.size());
        fixture.queue->EnqueueRequest(&request);
        fixture.Wait();
        Require(fixture.status->GetHResult(0) == 0, "Physical file destination request failed");
        auto output = memory.subspan(request.destinationPageOffset, data.plain.size());
        Require(std::equal(data.plain.begin(), data.plain.end(), output.begin()), "Physical file request changed destination page order or offset");
        Require(memory[request.destinationPageOffset - 1] == std::byte{0x57} && memory[request.destinationPageOffset + data.plain.size()] == std::byte{0x57}, "Physical file request overwrote a destination guard");
    }
}
