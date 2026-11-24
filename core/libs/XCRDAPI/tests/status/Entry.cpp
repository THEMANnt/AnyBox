#include "../file/Tests.hpp"
#include "../../src/status/Entry.hpp"

#include <bit>
#include <semaphore>
#include <thread>

void CheckStatusEntry() {
    using namespace AnyBox::Tests;
    using namespace AnyBox::Storage;
    StatusEntry entry;
    auto first = entry.Begin();
    Require(!entry.IsComplete() && std::bit_cast<std::uint32_t>(entry.GetResult()) == PendingResult, "Status entry did not become pending");
    ExpectException<std::logic_error>([&] { entry.Begin(); });
    ExpectException<std::invalid_argument>([&] { entry.Complete(first, std::bit_cast<std::int32_t>(PendingResult)); });
    auto failure = std::bit_cast<std::int32_t>(std::uint32_t{0x80070026});
    entry.Complete(first, failure);
    Require(entry.IsComplete() && entry.GetResult() == failure, "Status completion lost its failure code");
    auto second = entry.Begin();
    ExpectException<std::logic_error>([&] { entry.Complete(first, 0); });
    Require(!entry.IsComplete(), "Stale completion changed a reused status entry");
    entry.Cancel(second);
    Require(entry.IsComplete() && entry.GetResult() == failure, "Canceled enqueue did not restore prior status");
    ExpectException<std::logic_error>([&] { entry.Cancel(second); });
    std::binary_semaphore ready{0};
    std::binary_semaphore consumed{0};
    StatusTicket ticket{};
    int payload = 0;
    std::jthread producer([&] {
        for (int i = 1; i <= 1024; ++i) {
            ready.acquire();
            payload = i;
            entry.Complete(ticket, 0);
            consumed.acquire();
        }
    });
    for (int i = 1; i <= 1024; ++i) {
        ticket = entry.Begin();
        ready.release();
        while (!entry.IsComplete()) std::this_thread::yield();
        auto correct = payload == i && entry.GetResult() == 0;
        consumed.release();
        if (!correct) std::terminate();
    }
}
