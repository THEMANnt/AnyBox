#include "Tests.hpp"

#include <array>
#include <string_view>

void AnyBox::Tests::CheckQueueInformation() {
    QueueFixture test(1);
    StorageQueueInformation information{};
    StorageQueueInformation1 information1{};
    test.queue->Query(&information);
    test.queue->Query1(&information1);
    Require(information.emptySlotCount == 127, "Empty queue must reserve one slot");
    Require(information.requestCountUntilAutoSubmit == 64, "Incorrect automatic submission threshold");
    Require(information.description.sourceType == 1 && information.description.priority == 2, "Queue query changed source or priority");
    Require(information.description.capacity == 128, "Queue query changed capacity");
    Require(std::string_view(information.description.name) == "Queue test", "Queue query changed name");
    Require(information1.description.options == 1 && information1.emptySlotCount == 127, "Extended queue query lost options or capacity");

    std::byte source{0x67};
    std::array<std::byte, 127> destination{};
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = &source;
    request.sourceSize = 1;
    request.destinationSize = 1;
    for (std::size_t i = 0; i < destination.size(); ++i) {
        request.destination = &destination[i];
        test.queue->EnqueueRequest(&request);
        if (i == 30) {
            test.queue->Query(&information);
            Require(information.emptySlotCount == 96, "Queue query counted occupied slots incorrectly");
            Require(information.requestCountUntilAutoSubmit == 33, "Queue query counted unpublished entries incorrectly");
        }
    }
    test.queue->Query1(&information1);
    Require(information1.emptySlotCount == 0, "Full queue reported free slots");
    test.queue->Submit();
    test.Wait();
    test.queue->Query(&information);
    Require(information.emptySlotCount == 127 && information.requestCountUntilAutoSubmit == 64, "Completed queue did not recover capacity");
    for (auto value : destination) Require(value == source, "Query affected queued reads");

    ExpectException<std::invalid_argument>([&] { test.queue->Query(nullptr); });
    ExpectException<std::invalid_argument>([&] { test.queue->Query1(nullptr); });
    test.queue->Close();
    ExpectException<std::logic_error>([&] { test.queue->Query(&information); });
    ExpectException<std::logic_error>([&] { test.queue->Query1(&information1); });
}
