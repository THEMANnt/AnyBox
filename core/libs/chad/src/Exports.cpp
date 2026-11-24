#include "runtime/Stream.hpp"

#include <chad.hpp>
#include <anybox/Unimplemented.hpp>

#include <limits>
#include <stdexcept>

std::int32_t XDspConnectWithMaximumStreamLimit(void* memory, std::uint32_t size, std::uint32_t impulseResponseSeconds, std::uint32_t maximumStreamCount, std::uint32_t mode, XDspConnection** connection) noexcept(false) {
    if (!connection) throw std::invalid_argument("DSP client output is null");
    if (!memory || size < 0x10000 || size > 0x1F400000) throw std::invalid_argument("Invalid DSP client storage");
    if (size > std::numeric_limits<std::uintptr_t>::max() - reinterpret_cast<std::uintptr_t>(memory)) throw std::overflow_error("DSP client storage range overflows");
    if (impulseResponseSeconds > 128 || maximumStreamCount == 0) throw std::invalid_argument("Invalid DSP client limits");
    if (maximumStreamCount > 256 || mode != 1) ANYBOX_NOT_IMPLEMENTED();

    auto created = std::make_shared<AnyBox::Dsp::Connection>(memory, size, impulseResponseSeconds, maximumStreamCount);
    *connection = AnyBox::Dsp::RegisterConnection(std::move(created));
    return 0;
}

std::int32_t XDspActivate(XDspConnection* connection, const XDspActivationParameters* parameters, XDspStatus** status, XDspStream** stream) noexcept(false) {
    if (!parameters || !status || !stream) throw std::invalid_argument("DSP activation argument is null");
    auto owner = AnyBox::Dsp::GetConnection(connection);
    auto created = std::make_shared<AnyBox::Dsp::Stream>(std::move(owner), *parameters);
    auto* sharedStatus = created->GetStatus();
    auto* handle = AnyBox::Dsp::RegisterStream(std::move(created));
    *status = sharedStatus;
    *stream = handle;
    return 0;
}

std::int32_t XDspDeactivate(XDspStream* stream) noexcept(false) {
    AnyBox::Dsp::RemoveStream(stream);
    return 0;
}

std::int32_t XDspDisconnect(XDspConnection* connection) noexcept(false) {
    AnyBox::Dsp::RemoveConnection(connection);
    return 0;
}

std::int32_t XDspSubmitCommand(XDspStream* stream, const XDspCommand* command, std::uint32_t* submittedCount) noexcept(false) {
    if (!command) throw std::invalid_argument("DSP command is null");
    auto sequence = AnyBox::Dsp::GetStream(stream)->Submit(*command);
    if (submittedCount) *submittedCount = sequence;
    return 0;
}
