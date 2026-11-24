#ifndef ANYBOX_ACPHAL_CHANNEL_HPP
#define ANYBOX_ACPHAL_CHANNEL_HPP

#include <IAcpHal.hpp>

#include <chrono>
#include <cstddef>
#include <mutex>
#include <vector>

namespace AnyBox::AcpHal {

class Channel final : public IAcpHal {
public:
    std::int32_t Connect(std::uint32_t commandCapacity, std::uint32_t messageCapacity) override;
    std::int32_t Disconnect() override;
    std::int32_t SubmitCommand(std::uint32_t type, std::uint64_t id, std::uint32_t frame, const void* data, std::uint32_t flags) override;
    bool PopMessage(AcpHalMessage* message) override;
    std::uint32_t GetNumMessages() override;
    void Release() override;

private:
    using Clock = std::chrono::steady_clock;
    static AcpHalMessage createMessage(std::uint32_t type, std::uint32_t frame, std::uint64_t id);
    void push(AcpHalMessage message);
    bool frameDue(Clock::time_point now) const;
    std::mutex mutex;
    std::vector<AcpHalMessage> messages;
    std::size_t head = 0;
    std::size_t count = 0;
    bool connected = false;
    std::uint32_t frameSamples = 0;
    std::uint64_t frameNumber = 0;
    Clock::time_point nextFrame{};
    Clock::duration framePeriod{};
};

}

#endif
