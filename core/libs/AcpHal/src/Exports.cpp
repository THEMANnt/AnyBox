#include <AcpHal.hpp>
#include "channel/Channel.hpp"

#include <stdexcept>

std::int32_t AcpHalCreate(IAcpHal** acpHal) noexcept(false) {
    if (!acpHal) throw std::invalid_argument("ACP channel output is null");
    auto* channel = new AnyBox::AcpHal::Channel();
    *acpHal = channel;
    return 0;
}
