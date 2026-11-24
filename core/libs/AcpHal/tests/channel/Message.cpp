#include "../memory/Tests.hpp"

#include <IAcpHal.hpp>

#include <array>
#include <limits>

void CheckMessageLayout() {
    using namespace AnyBox::Tests;
    struct Output {
        std::array<std::byte, 4> before;
        AcpHalMessage message;
        std::array<std::byte, 4> after;
    };
    static_assert(sizeof(Output) == 36);
    Output output{};
    output.before.fill(std::byte{0x57});
    output.after.fill(std::byte{0x57});
    output.message.data1 = 0x57575757;
    IAcpHal* channel = nullptr;
    AcpHalCreate(&channel);
    channel->Connect(4, 4);
    std::uint32_t samples = 48000;
    constexpr std::uint64_t identifier = 0x0123456789ABCDEF;
    channel->SubmitCommand(2, identifier, std::numeric_limits<std::uint32_t>::max(), &samples, 0);
    Require(channel->PopMessage(&output.message), "ACP completion message is missing");
    Require(output.message.type == 0x10 && output.message.id.Get() == identifier, "ACP completion message identifier changed");
    Require(output.message.data1 == 0, "ACP PopMessage did not initialize the final payload word");
    for (auto value : output.before) Require(value == std::byte{0x57}, "ACP message write underflowed its output");
    for (auto value : output.after) Require(value == std::byte{0x57}, "ACP message write exceeded the original 28-byte ABI");
    channel->Disconnect();
    channel->Release();
}
