#include "Channel.hpp"
#include "../memory/Heap.hpp"

#include <stdexcept>

std::int32_t AnyBox::AcpHal::Channel::Connect(std::uint32_t commandCapacity, std::uint32_t messageCapacity) {
    if (commandCapacity == 0 || messageCapacity < 2) throw std::invalid_argument("ACP channel capacities are invalid");
    GetHeap().Initialize();
    std::lock_guard lock(mutex);
    if (connected) throw std::logic_error("ACP channel is already connected");
    if (count != 0) throw std::logic_error("ACP channel has undrained messages");
    std::vector<AcpHalMessage> storage(messageCapacity);
    messages.swap(storage);
    head = 0;
    frameSamples = 0;
    frameNumber = 0;
    connected = true;
    return 0;
}

std::int32_t AnyBox::AcpHal::Channel::Disconnect() {
    std::lock_guard lock(mutex);
    if (!connected) throw std::logic_error("ACP channel is not connected");
    push(createMessage(0x80, static_cast<std::uint32_t>(frameNumber), 0));
    connected = false;
    frameSamples = 0;
    return 0;
}

void AnyBox::AcpHal::Channel::Release() {
    {
        std::lock_guard lock(mutex);
        if (connected) throw std::logic_error("ACP channel must be disconnected before release");
    }
    delete this;
}
