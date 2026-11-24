#include "Clock.hpp"
#include <atomic>
#include <limits>

namespace {
std::uint64_t InitialToken() {
    static std::atomic<std::uint64_t> next{1};
    auto sequence = next.fetch_add(1, std::memory_order_relaxed);
    if (sequence > std::numeric_limits<std::uint16_t>::max()) throw std::overflow_error("Xbox frame clock identity is exhausted");
    return sequence << 48;
}
}

AnyBox::Graphics::FrameClock::FrameClock() : wake(OwnFrameHandle(CreateEventW(nullptr, FALSE, FALSE, nullptr), "CreateEventW frame scheduler")), failed(OwnFrameHandle(CreateEventW(nullptr, TRUE, FALSE, nullptr), "CreateEventW frame scheduler failure")), timer(OwnFrameHandle(CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS), "CreateWaitableTimerExW Xbox frame clock")), nextToken(InitialToken()) {
    kinds[0].event = OwnFrameHandle(CreateSemaphoreW(nullptr, 0, 4, nullptr), "CreateSemaphoreW frame origins");
    for (std::size_t index = 1; index < kinds.size(); ++index) kinds[index].event = OwnFrameHandle(CreateEventW(nullptr, FALSE, FALSE, nullptr), "CreateEventW Xbox frame point");
    worker = std::jthread([this](std::stop_token stop) { run(stop); });
}

AnyBox::Graphics::FrameClock::~FrameClock() {
    worker.request_stop();
    if (!SetEvent(wake.get())) std::terminate();
}

void AnyBox::Graphics::FrameClock::checkFailure() const {
    if (failure) std::rethrow_exception(failure);
}

std::int64_t AnyBox::Graphics::FrameClock::counter() {
    LARGE_INTEGER value;
    if (!QueryPerformanceCounter(&value)) Windows::ThrowLastError("QueryPerformanceCounter Xbox frame clock");
    return value.QuadPart;
}
