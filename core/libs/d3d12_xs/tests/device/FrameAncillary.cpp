#include "Tests.hpp"
#include "../../src/frame/Ancillary.hpp"

namespace {
AnyBox::Graphics::FrameHandle Duplicate(HANDLE source, DWORD access = 0) {
    HANDLE output = nullptr;
    if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &output, access, FALSE, access == 0 ? DUPLICATE_SAME_ACCESS : 0)) AnyBox::Windows::ThrowLastError("DuplicateHandle frame test");
    return AnyBox::Graphics::OwnFrameHandle(output, "Own frame test duplicate");
}
}

void AnyBox::Graphics::Tests::CheckFrameAncillary(Device* device) {
    auto schedule = Method<decltype(&Device::ScheduleFrameEventX)>(device, 79);
    auto wait = Method<decltype(&Device::WaitFrameEventX)>(device, 80);
    auto event = OwnFrameHandle(CreateEventW(nullptr, TRUE, FALSE, nullptr), "CreateEventW frame signal test");
    auto semaphore = OwnFrameHandle(CreateSemaphoreW(nullptr, 0, 2, nullptr), "CreateSemaphoreW frame signal test");
    auto observedEvent = Duplicate(event.get());
    auto observedSemaphore = Duplicate(semaphore.get());
    const HANDLE sources[]{event.get(), semaphore.get()};
    FrameAncillary ancillary{2, sources, nullptr};
    Require(schedule(device, 2, 0, &ancillary, 0) == S_OK, "Frame ancillary schedule failed");
    event.reset();
    semaphore.reset();
    Require(WaitForSingleObject(observedEvent.get(), 1000) == WAIT_OBJECT_0, "Frame scheduler did not signal its retained event");
    Require(WaitForSingleObject(observedSemaphore.get(), 1000) == WAIT_OBJECT_0, "Frame scheduler did not signal its retained semaphore");
    Require(schedule(device, 2, 0, nullptr, 1) == S_OK, "Frame ancillary cancellation failed");
    if (!ResetEvent(observedEvent.get())) Windows::ThrowLastError("ResetEvent frame signal test");
    Require(WaitForSingleObject(observedEvent.get(), 80) == WAIT_TIMEOUT, "Cancelled frame ancillary event was signaled again");

    auto restricted = Duplicate(observedEvent.get(), SYNCHRONIZE);
    HANDLE source = restricted.get();
    FrameAncillary invalid{1, &source, nullptr};
    Reject<std::invalid_argument>([&] { schedule(device, 2, 0, &invalid, 0); });
    source = nullptr;
    Reject<std::invalid_argument>([&] { schedule(device, 2, 0, &invalid, 0); });

    source = observedEvent.get();
    std::int32_t signaled = -99;
    FrameAncillary external{1, &source, &signaled};
    if (!SetEvent(source)) Windows::ThrowLastError("SetEvent frame wait test");
    std::uint64_t token = 0;
    bool observed = false;
    for (unsigned index = 0; index < 16 && !observed; ++index) {
        Require(wait(device, 1, 1000, &external, 0, &token) == S_OK && token == 0, "Frame ancillary wait failed or modified its token");
        Require(signaled == -2 || signaled == 0, "Frame ancillary wait returned an invalid handle index");
        observed = signaled == 0;
    }
    Require(observed, "Frame ancillary wait did not observe its external event");
    Reject<std::runtime_error>([&] { wait(device, 1, 0, &external, 1, &token); });
}
