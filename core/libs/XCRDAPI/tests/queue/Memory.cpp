#include "Tests.hpp"

#include <array>
#include <cstddef>
#include <chrono>
#include <future>
#include <thread>

void AnyBox::Tests::CheckMemoryQueue() {
    std::array<std::byte, 257> source;
    std::array<std::array<std::byte, 257>, 32> destinations{};
    for (std::size_t i = 0; i < source.size(); ++i) source[i] = std::byte(i % 251);
    QueueFixture test(1);
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destinationSize = request.sourceSize;
    for (std::uint32_t batch = 0; batch < 32; ++batch) {
        for (auto& destination : destinations) {
            destination.fill(std::byte{0xFE});
            request.destination = destination.data();
            test.queue->EnqueueRequest(&request);
        }
        for (const auto& destination : destinations) Require(destination[0] == std::byte{0xFE}, "Queue executed an unpublished request");
        test.Wait();
        Require(test.status->GetHResult(0) == 0, "Memory request failed");
        for (const auto& destination : destinations) Require(destination == source, "Memory queue corrupted its request ordering or data");
    }
    void* unknown = nullptr;
    Require(test.queue->QueryInterface(Storage::UnknownId, &unknown) == 0 && unknown == test.queue.get(), "Queue identity changed");
    static_cast<IStorageUnknown*>(unknown)->Release();
    auto* output = test.queue.get();
    ExpectException<std::invalid_argument>([&] { test.factory->CreateQueue1(nullptr, Storage::QueueId, reinterpret_cast<void**>(&output)); });
    test.queue->Close();
    test.queue->Close();
    ExpectException<std::logic_error>([&] { test.queue->EnqueueRequest(&request); });
}

void AnyBox::Tests::CheckQueueLimits() {
    std::byte source{0x71};
    std::array<std::byte, 128> received{};
    QueueFixture test(1);
    StorageRequest request{};
    request.options = 1ULL << 22;
    request.source = &source;
    request.sourceSize = 1;
    request.destinationSize = 1;
    for (std::size_t i = 0; i < 127; ++i) {
        request.destination = &received[i];
        test.queue->EnqueueRequest(&request);
    }
    request.destination = &received.back();
    std::promise<void> started;
    std::promise<void> enqueued;
    auto start = started.get_future();
    auto completion = enqueued.get_future();
    std::exception_ptr enqueueError;
    std::jthread producer([&] {
        started.set_value();
        try {
            test.queue->EnqueueRequest(&request);
        } catch (...) {
            enqueueError = std::current_exception();
        }
        enqueued.set_value();
    });
    start.wait();
    auto blocked = completion.wait_for(std::chrono::milliseconds(20));
    test.queue->Submit();
    auto finished = completion.wait_for(std::chrono::seconds(5));
    producer.join();
    if (enqueueError) std::rethrow_exception(enqueueError);
    Require(blocked == std::future_status::timeout && finished == std::future_status::ready, "Full DirectStorage queue did not wait for Submit");
    test.queue->Close();
    for (auto byte : received) Require(byte == source, "Backpressured request was lost");
    ExpectException<std::logic_error>([&] { test.queue->Submit(); });
    StorageQueueDescription invalid{1, 127, 0, {}, "Invalid"};
    void* output = nullptr;
    ExpectException<std::invalid_argument>([&] { test.factory->CreateQueue(&invalid, Storage::QueueId, &output); });
    invalid.capacity = 128;
    ExpectException<std::invalid_argument>([&] { test.factory->CreateQueue(&invalid, Storage::QueueId, &output); });
}
