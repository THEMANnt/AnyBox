#include "Tests.hpp"
#include "../../src/staging/Pool.hpp"
#include "../../src/metrics/Counters.hpp"

#include <chrono>
#include <future>
#include <optional>
#include <thread>

void AnyBox::Tests::CheckStaging() {
    Storage::StagingPool pool;
    ExpectException<std::invalid_argument>([&] { pool.SetSize(1024); });
    pool.SetSize(16 * 1024 * 1024);
    {
        auto first = pool.Acquire(100);
        auto second = pool.Acquire(100);
        Require(first.Get().data() != second.Get().data(), "Staging leases overlap");
        first.Get()[0] = std::byte{0x17};
        second.Get()[0] = std::byte{0x39};
        Require(first.Get()[0] == std::byte{0x17}, "Staging lease writes overlap");
        ExpectException<std::logic_error>([&] { pool.SetSize(0); });
    }

    auto stallsBefore = Storage::GetCounters().stagingStalls.load();
    std::optional<Storage::StagingLease> full(pool.Acquire(16 * 1024 * 1024));
    std::promise<void> started;
    std::promise<void> acquired;
    auto start = started.get_future();
    auto completion = acquired.get_future();
    std::jthread waiter([&] {
        started.set_value();
        auto lease = pool.Acquire(1);
        lease.Get()[0] = std::byte{0x51};
        acquired.set_value();
    });
    start.wait();
    auto blocked = completion.wait_for(std::chrono::milliseconds(20));
    full.reset();
    auto finished = completion.wait_for(std::chrono::seconds(5));
    waiter.join();
    Require(Storage::GetCounters().stagingStalls.load() == stallsBefore + 1, "Staging exhaustion was not counted exactly once");
    Require(blocked == std::future_status::timeout && finished == std::future_status::ready, "Staging leases do not wait for available capacity");

    pool.RegisterObject();
    ExpectException<std::logic_error>([&] { pool.SetSize(0); });
    pool.UnregisterObject();
    pool.SetSize(0);
    ExpectException<std::length_error>([&] { pool.Acquire(1); });
    auto factory = GetFactory();
    Require(factory->SetStagingBufferSize(0) == 0, "Factory could not disable staging without live objects");
    {
        QueueFixture queue(1);
        ExpectException<std::logic_error>([&] { factory->SetStagingBufferSize(32 * 1024 * 1024); });
    }
    Require(factory->SetStagingBufferSize(32 * 1024 * 1024) == 0, "Factory did not retire its queue registration");
}
