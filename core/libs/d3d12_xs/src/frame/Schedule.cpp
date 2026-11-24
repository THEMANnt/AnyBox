#include "Clock.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::FrameClock::Schedule(FrameConfiguration requested, std::uint32_t type, std::uint32_t offset, const FrameAncillary* ancillary, std::uint32_t flags) {
    if (type >= kinds.size() || requested.intervalMicroseconds == 0 || flags > 1) throw std::invalid_argument("Xbox frame event type, interval or flags are invalid");
    FrameSignals signals(flags == 0 ? ancillary : nullptr);
    std::scoped_lock lock(mutex);
    checkFailure();
    if (!kinds[0].scheduled && (type != 0 || flags != 0)) throw std::invalid_argument("Xbox frame origin must be scheduled first");
    if (flags != 0) {
        if (type == 0) throw std::invalid_argument("Xbox frame origin cannot be unscheduled");
        auto previous = kinds[type].scheduled;
        kinds[type].scheduled = false;
        kinds[type].signals = std::move(signals);
        SignalFrameEvent(wake.get());
        return previous ? S_OK : S_FALSE;
    }

    auto now = std::chrono::steady_clock::now();
    if (configuration != requested) {
        configuration = requested;
        base = now;
        for (auto& kind : kinds) {
            kind.offset %= configuration.intervalMicroseconds;
            if (kind.scheduled) kind.deadline = NextFrameDeadline(base, std::chrono::microseconds(configuration.intervalMicroseconds), std::chrono::microseconds(kind.offset), now);
        }
    }
    auto& kind = kinds[type];
    auto unchanged = kind.scheduled && kind.offset == offset % configuration.intervalMicroseconds;
    kind.offset = offset % configuration.intervalMicroseconds;
    if (!unchanged) kind.deadline = NextFrameDeadline(base, std::chrono::microseconds(configuration.intervalMicroseconds), std::chrono::microseconds(kind.offset), now);
    kind.signals = std::move(signals);
    kind.scheduled = true;
    SignalFrameEvent(wake.get());
    return unchanged ? S_FALSE : S_OK;
}
