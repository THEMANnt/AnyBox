#ifndef ANYBOX_CHAD_HPP
#define ANYBOX_CHAD_HPP

#include <chadTypes.hpp>

#include <cstdint>

extern "C" {

std::int32_t XDspActivate(XDspConnection* connection, const XDspActivationParameters* parameters, XDspStatus** status, XDspStream** stream) noexcept(false);
std::int32_t XDspConnectWithMaximumStreamLimit(void* memory, std::uint32_t size, std::uint32_t impulseResponseSeconds, std::uint32_t maximumStreamCount, std::uint32_t mode, XDspConnection** connection) noexcept(false);
std::int32_t XDspDeactivate(XDspStream* stream) noexcept(false);
std::int32_t XDspDisconnect(XDspConnection* connection) noexcept(false);
std::int32_t XDspSubmitCommand(XDspStream* stream, const XDspCommand* command, std::uint32_t* submittedCount) noexcept(false);

}

#endif
