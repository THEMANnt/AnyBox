#include "Tests.hpp"

#include <algorithm>
#include <array>
#include <limits>

void AnyBox::Tests::CheckPhysicalLifetime() {
    PhysicalBuffer source(2);
    PhysicalBuffer destination(2);
    auto input = source.GetMemory();
    std::fill(input.begin(), input.end(), std::byte{0x73});
    source.Unmap();
    destination.Unmap();
    auto from = source.GetFrames();
    auto to = destination.GetFrames();
    std::array<std::uint64_t, 2> inputArray{from[0], from[1]};
    std::array<std::uint64_t, 2> outputArray{to[0], to[1]};
    QueueFixture fixture(1);
    StorageRequest request{};
    request.options = (1ULL << 21) | (1ULL << 22) | (1ULL << 23);
    request.source = inputArray.data();
    request.sourceSize = static_cast<std::uint32_t>(2 * AnyBox::PhysicalPageSize);
    request.destination = outputArray.data();
    request.destinationSize = request.sourceSize;
    fixture.queue->EnqueueRequest(&request);
    inputArray.fill(0);
    outputArray.fill(0);
    ExpectException<std::logic_error>([&] { source.FreePages(); });
    ExpectException<std::logic_error>([&] { destination.FreePages(); });
    fixture.Wait();
    Require(fixture.status->GetHResult(0) == 0, "Captured physical page arrays were not retained");
    destination.Map(to);
    Require(std::all_of(destination.GetMemory().begin(), destination.GetMemory().end(), [](auto value) { return value == std::byte{0x73}; }), "Retained physical pages changed after enqueue");
    destination.Unmap();
    source.FreePages();
    destination.FreePages();

    PhysicalBuffer rejected(2);
    rejected.Unmap();
    auto valid = rejected.GetFrames();
    std::array<std::uint64_t, 2> invalid{valid[0], std::numeric_limits<std::uint64_t>::max()};
    request.source = invalid.data();
    request.destination = const_cast<std::uint64_t*>(valid.data());
    ExpectException<std::invalid_argument>([&] { fixture.queue->EnqueueRequest(&request); });
    rejected.FreePages();
}

void AnyBox::Tests::CheckRequestPadding() {
    auto factory = GetFactory();
    StorageQueueDescription1 description{{1, 128, 2, {}, "Padding"}, 1};
    description.description.padding.fill(0xA5);
    void* output = nullptr;
    factory->CreateQueue1(&description, Storage::QueueId, &output);
    Object<IStorageQueue> queue(static_cast<IStorageQueue*>(output));
    StorageQueueInformation information{};
    queue->Query(&information);
    Require(information.description.sourceType == 1 && information.description.capacity == 128 && information.description.priority == 2, "Queue ABI padding changed its logical fields");
}
