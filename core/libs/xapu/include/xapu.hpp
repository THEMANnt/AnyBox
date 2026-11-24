#ifndef ANYBOX_XAPU_HPP
#define ANYBOX_XAPU_HPP

#include <xapuTypes.hpp>

#include <cstdint>

extern "C" {

std::int32_t XApuConnect(const XApuConnectionParameters* parameters, XApuBuffer* buffer, XApuConnection** connection) noexcept(false);
std::int32_t XApuDequeueResult(XApuConnection* connection, XApuResult* result) noexcept(false);
std::int32_t XApuDisconnect(XApuConnection* connection) noexcept(false);
std::int32_t XApuEnqueueCommand(XApuConnection* connection, const XApuCommand* command, std::uint32_t* submittedCount) noexcept(false);
std::uint32_t XApuGetResultCount(XApuConnection* connection, std::uint32_t* dequeuedCount) noexcept(false);

}

#endif
