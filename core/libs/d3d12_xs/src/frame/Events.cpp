#include "Clock.hpp"
#include "../device/Device.hpp"

HRESULT AnyBox::Graphics::Device::ScheduleFrameEventX(Device* self, std::uint32_t type, std::uint32_t offsetMicroseconds, const FrameAncillary* ancillary, std::uint32_t flags) {
    std::scoped_lock lock(self->frameMutex);
    if (self->frameConfiguration.intervalMicroseconds == 0) throw std::invalid_argument("Xbox frame interval must be configured before scheduling events");
    if (self->frameClock) return self->frameClock->Schedule(self->frameConfiguration, type, offsetMicroseconds, ancillary, flags);
    if (type != 0 || flags != 0) throw std::invalid_argument("Xbox frame origin must be scheduled first");
    auto clock = std::make_unique<FrameClock>();
    auto result = clock->Schedule(self->frameConfiguration, type, offsetMicroseconds, ancillary, flags);
    self->frameClock = clock.release();
    return result;
}

HRESULT AnyBox::Graphics::Device::WaitFrameEventX(Device* self, std::uint32_t type, std::uint32_t timeoutMilliseconds, const FrameAncillary* ancillary, std::uint32_t flags, std::uint64_t* token) {
    FrameClock* clock;
    {
        std::scoped_lock lock(self->frameMutex);
        clock = self->frameClock;
    }
    if (!clock) throw std::invalid_argument("Xbox frame events must be scheduled before waiting");
    return clock->Wait(type, timeoutMilliseconds, ancillary, flags, token);
}
