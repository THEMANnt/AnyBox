#include "Channel.hpp"
#include "../shape/Runtime.hpp"

#include <anybox/Unimplemented.hpp>

#include <cstring>
#include <limits>
#include <stdexcept>

std::int32_t AnyBox::AcpHal::Channel::SubmitCommand(std::uint32_t type, std::uint64_t id, std::uint32_t frame, const void* data, std::uint32_t flags) {
    if (frame != std::numeric_limits<std::uint32_t>::max() || flags != 0) throw std::invalid_argument("Unsupported ACP command scheduling");
    if (!data) throw std::invalid_argument("ACP command payload is null");
    std::lock_guard lock(mutex);
    if (!connected) throw std::logic_error("ACP channel is not connected");
    if (type != 2 && type != 5 && type != 7 && type != 18) ANYBOX_NOT_IMPLEMENTED();
    if (count == messages.size()) throw std::overflow_error("ACP message queue is full");
    if (type != 2) {
        SubmitShapeCommand(type, data);
        push(createMessage(0x10, static_cast<std::uint32_t>(frameNumber), id));
        return 0;
    }
    std::uint32_t samples;
    std::memcpy(&samples, data, sizeof(samples));
    if (samples == 0 || samples > 48000) throw std::invalid_argument("ACP frame length is invalid");
    auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<std::int64_t, std::ratio<1, 48000>>(samples));
    if (period <= Clock::duration::zero()) throw std::invalid_argument("ACP frame period is below the clock resolution");
    push(createMessage(0x10, static_cast<std::uint32_t>(frameNumber), id));
    frameSamples = samples;
    framePeriod = period;
    frameNumber = 0;
    nextFrame = Clock::now() + period;
    return 0;
}
