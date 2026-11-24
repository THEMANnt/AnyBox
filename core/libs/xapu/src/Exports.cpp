#include "connection/Connection.hpp"

#include <xapu.hpp>
#include <anybox/Unimplemented.hpp>

#include <stdexcept>

std::int32_t XApuConnect(const XApuConnectionParameters* parameters, XApuBuffer* buffer, XApuConnection** connection) noexcept(false) {
    if (!connection) throw std::invalid_argument("XApu connection output is null");
    *connection = nullptr;
    if (!buffer) throw std::invalid_argument("XApu buffer output is null");
    *buffer = {};

    if (!parameters) throw std::invalid_argument("XApu connection parameters are null");
    if (parameters->processor < 1 || parameters->processor > 3) throw std::invalid_argument("Invalid XApu processor");
    if (parameters->streamCount == 0 || parameters->streamCount > 500) throw std::invalid_argument("Invalid XApu stream count");
    if (parameters->commandsPerStream == 0 || parameters->commandsPerStream > 16) throw std::invalid_argument("Invalid XApu command count");
    if (parameters->bufferSize > 0x20000000) throw std::length_error("XApu connection buffer exceeds its limit");
    if ((parameters->flags & ~15U) != 0) throw std::invalid_argument("Invalid XApu connection flags");
    if (parameters->processor != 1 || (parameters->flags & ~1U) != 0) ANYBOX_NOT_IMPLEMENTED();

    auto created = std::make_shared<AnyBox::XApu::Connection>(*parameters);
    auto* handle = AnyBox::XApu::RegisterConnection(created);
    buffer->data = created->GetBuffer();
    *connection = handle;
    return 0;
}

std::int32_t XApuDequeueResult(XApuConnection* connection, XApuResult* result) noexcept(false) {
    AnyBox::XApu::GetConnection(connection)->Dequeue(result);
    return 0;
}

std::int32_t XApuDisconnect(XApuConnection* connection) noexcept(false) {
    AnyBox::XApu::RemoveConnection(connection);
    return 0;
}

std::int32_t XApuEnqueueCommand(XApuConnection* connection, const XApuCommand* command, std::uint32_t* submittedCount) noexcept(false) {
    auto count = AnyBox::XApu::GetConnection(connection)->Enqueue(command);
    if (submittedCount) *submittedCount = count;
    return 0;
}

std::uint32_t XApuGetResultCount(XApuConnection* connection, std::uint32_t* dequeuedCount) noexcept(false) {
    return AnyBox::XApu::GetConnection(connection)->Count(dequeuedCount);
}
