#include "Channel.hpp"

#include <stdexcept>

AcpHalMessage AnyBox::AcpHal::Channel::createMessage(std::uint32_t type, std::uint32_t frame, std::uint64_t id) {
    AcpHalMessage message{};
    message.type = type;
    message.frame = frame;
    message.id.Set(id);
    return message;
}

void AnyBox::AcpHal::Channel::push(AcpHalMessage message) {
    if (count == messages.size()) throw std::overflow_error("ACP message queue is full");
    messages[(head + count) % messages.size()] = message;
    ++count;
}

bool AnyBox::AcpHal::Channel::frameDue(Clock::time_point now) const {
    return connected && frameSamples != 0 && now >= nextFrame;
}

bool AnyBox::AcpHal::Channel::PopMessage(AcpHalMessage* message) {
    if (!message) throw std::invalid_argument("ACP message output is null");
    std::lock_guard lock(mutex);
    if (count != 0) {
        *message = messages[head];
        head = (head + 1) % messages.size();
        --count;
        return true;
    }
    auto now = Clock::now();
    if (!frameDue(now)) return false;
    auto elapsed = (now - nextFrame) / framePeriod + 1;
    frameNumber += static_cast<std::uint64_t>(elapsed);
    nextFrame += framePeriod * elapsed;
    *message = createMessage(2, static_cast<std::uint32_t>(frameNumber), 0);
    return true;
}

std::uint32_t AnyBox::AcpHal::Channel::GetNumMessages() {
    std::lock_guard lock(mutex);
    return static_cast<std::uint32_t>(count) + (frameDue(Clock::now()) ? 1U : 0U);
}
