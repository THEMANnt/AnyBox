#include "../memory/Tests.hpp"

#include <IAcpHal.hpp>

#include <chrono>
#include <iostream>
#include <limits>
#include <thread>

void CheckConcurrentChannel();
void CheckMessageLayout();

int main() {
    try {
        using namespace AnyBox::Tests;
        IAcpHal* channel = nullptr;
        Require(AcpHalCreate(&channel) == 0 && channel != nullptr, "ACP factory did not create a channel");
        AcpHalMessage message{0x1234, 0, 0, 0, {}, 0};
        Require(!channel->PopMessage(&message) && message.type == 0x1234, "Empty ACP queue changed its output");
        ExpectException<std::invalid_argument>([] { AcpHalCreate(nullptr); });
        ExpectException<std::invalid_argument>([&] { channel->Connect(0, 2); });
        Require(channel->Connect(4, 4) == 0, "ACP channel connection failed");
        ExpectException<std::logic_error>([] { ApuCreateHeap(0x100000, 0); });
        ExpectException<std::logic_error>([&] { channel->Connect(4, 4); });
        ExpectException<std::logic_error>([&] { channel->Release(); });
        std::uint32_t samples = 192;
        auto immediate = std::numeric_limits<std::uint32_t>::max();
        ExpectException<std::invalid_argument>([&] { channel->SubmitCommand(2, 1, 0, &samples, 0); });
        ExpectException<std::invalid_argument>([&] { channel->SubmitCommand(2, 1, immediate, nullptr, 0); });
        ExpectException<std::logic_error>([&] { channel->SubmitCommand(18, 1, immediate, &samples, 0); });
        Require(channel->GetNumMessages() == 0, "Rejected ACP command queued completion");
        for (std::uint64_t i = 0; i < 4; ++i) Require(channel->SubmitCommand(2, i + 1, immediate, &samples, 0) == 0, "ACP configuration failed");
        ExpectException<std::overflow_error>([&] { channel->SubmitCommand(2, 5, immediate, &samples, 0); });
        for (std::uint64_t i = 0; i < 4; ++i) {
            Require(channel->PopMessage(&message), "ACP completion was lost");
            Require(message.type == 0x10 && message.id.Get() == i + 1, "ACP completion order changed");
        }
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        bool frame = false;
        while (std::chrono::steady_clock::now() < deadline) {
            if (channel->PopMessage(&message)) {
                frame = message.type == 2 && message.frame > 0;
                break;
            }
            std::this_thread::yield();
        }
        Require(frame, "ACP frame clock did not advance");
        channel->Disconnect();
        Require(channel->PopMessage(&message) && message.type == 0x80, "ACP disconnect notification was lost");
        Require(!channel->PopMessage(&message), "Disconnected ACP clock still produced messages");
        Require(channel->Connect(2, 2) == 0, "ACP channel reconnect failed");
        channel->Disconnect();
        ExpectException<std::logic_error>([&] { channel->Connect(2, 2); });
        Require(channel->PopMessage(&message) && message.type == 0x80, "Reconnect lost its disconnect notification");
        channel->Release();
        CheckConcurrentChannel();
        CheckMessageLayout();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
