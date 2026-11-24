#ifndef ANYBOX_IACPHAL_HPP
#define ANYBOX_IACPHAL_HPP

#include <anybox/AbiValue.hpp>

#include <cstddef>
#include <cstdint>

struct AcpHalMessage {
    std::uint32_t type;
    std::uint32_t frame;
    std::uint32_t timeMicroseconds;
    std::uint32_t data0;
    AnyBox::AbiValue<std::uint64_t> id;
    std::uint32_t data1;
};

struct IAcpHal {
    virtual std::int32_t Connect(std::uint32_t commandCapacity, std::uint32_t messageCapacity) = 0;
    virtual std::int32_t Disconnect() = 0;
    virtual std::int32_t SubmitCommand(std::uint32_t type, std::uint64_t id, std::uint32_t frame, const void* data, std::uint32_t flags) = 0;
    virtual bool PopMessage(AcpHalMessage* message) = 0;
    virtual std::uint32_t GetNumMessages() = 0;
    virtual void Release() = 0;
};

static_assert(sizeof(AcpHalMessage) == 28);
static_assert(alignof(AcpHalMessage) == 4);
static_assert(offsetof(AcpHalMessage, frame) == 4);
static_assert(offsetof(AcpHalMessage, timeMicroseconds) == 8);
static_assert(offsetof(AcpHalMessage, id) == 16);
static_assert(offsetof(AcpHalMessage, data1) == 24);

#endif
