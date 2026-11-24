#include "Clock.hpp"
#include <algorithm>

void AnyBox::Graphics::FrameClock::run(std::stop_token stop) noexcept {
    try {
        while (!stop.stop_requested()) tick();
    } catch (...) {
        std::scoped_lock lock(mutex);
        failure = std::current_exception();
        if (!SetEvent(failed.get())) std::terminate();
    }
}

void AnyBox::Graphics::FrameClock::tick() {
    auto deadline = FrameTime::max();
    {
        std::scoped_lock lock(mutex);
        auto now = std::chrono::steady_clock::now();
        for (std::size_t index = 0; index < kinds.size(); ++index) {
            auto& kind = kinds[index];
            if (!kind.scheduled) continue;
            if (kind.deadline <= now) {
                publish(index);
                kind.deadline = NextFrameDeadline(base, std::chrono::microseconds(configuration.intervalMicroseconds), std::chrono::microseconds(kind.offset), now);
            }
            deadline = std::min(deadline, kind.deadline);
        }
    }
    if (deadline == FrameTime::max()) {
        auto result = WaitForSingleObject(wake.get(), INFINITE);
        if (result == WAIT_FAILED) Windows::ThrowLastError("WaitForSingleObject frame scheduler");
        if (result != WAIT_OBJECT_0) throw std::runtime_error("Unexpected frame scheduler wake result");
        return;
    }

    using TimerTicks = std::chrono::duration<std::int64_t, std::ratio<1, 10000000>>;
    auto ticks = std::max<std::int64_t>(1, std::chrono::ceil<TimerTicks>(deadline - std::chrono::steady_clock::now()).count());
    LARGE_INTEGER due;
    due.QuadPart = -ticks;
    if (!SetWaitableTimer(timer.get(), &due, 0, nullptr, nullptr, FALSE)) Windows::ThrowLastError("SetWaitableTimer Xbox frame clock");
    const HANDLE handles[]{wake.get(), timer.get()};
    auto result = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    if (result == WAIT_FAILED) Windows::ThrowLastError("WaitForMultipleObjects frame scheduler");
    if (result != WAIT_OBJECT_0 && result != WAIT_OBJECT_0 + 1) throw std::runtime_error("Unexpected frame timer wait result");
}

void AnyBox::Graphics::FrameClock::publish(std::size_t type) {
    if (type == 0) {
        auto qpc = counter();
        if (SignalFrameSemaphore(kinds[type].event.get())) {
            if (originCount >= origins.size()) throw std::logic_error("Xbox frame origin queue exceeded its semaphore capacity");
            origins[(originBegin + originCount) % origins.size()] = {qpc};
            ++originCount;
        }
    } else {
        SignalFrameEvent(kinds[type].event.get());
    }
    kinds[type].signals.Signal();
}
