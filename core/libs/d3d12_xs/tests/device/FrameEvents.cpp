#include "Tests.hpp"
#include "../../src/frame/Ancillary.hpp"

void AnyBox::Graphics::Tests::CheckFrameEvents(Device* device) {
    auto configure = Method<decltype(&Device::SetFrameIntervalX)>(device, 78);
    auto schedule = Method<decltype(&Device::ScheduleFrameEventX)>(device, 79);
    auto wait = Method<decltype(&Device::WaitFrameEventX)>(device, 80);
    Require(configure(device, nullptr, 66667, 3, 0) == S_FALSE, "Frame event interval configuration failed");
    std::uint64_t token = 0;
    Reject<std::invalid_argument>([&] { wait(device, 0, 0, nullptr, 0, &token); });
    Reject<std::invalid_argument>([&] { schedule(device, 1, 0, nullptr, 0); });
    FrameAncillary invalid{8, nullptr, nullptr};
    Reject<std::invalid_argument>([&] { schedule(device, 0, 0, &invalid, 0); });
    Require(schedule(device, 0, 0, nullptr, 0) == S_OK, "Frame origin scheduling failed");
    Require(schedule(device, 0, 0, nullptr, 0) == S_FALSE, "Repeated frame origin scheduling did not retain its phase");
    Require(schedule(device, 1, 1000, nullptr, 0) == S_OK, "First frame point scheduling failed");
    Require(schedule(device, 2, 2000, nullptr, 0) == S_OK, "Second frame point scheduling failed");
    Require(wait(device, 0, 1000, nullptr, 0, &token) == S_OK && token != 0, "Frame origin wait did not issue a token");
    auto previous = token;
    Reject<std::invalid_argument>([&] { wait(device, 0, 0, nullptr, 0, &token); });
    token = 0;
    Require(wait(device, 0, 1000, nullptr, 1, &token) == S_OK && token > previous, "Frame origin tokens did not advance");
    token = 0x123456;
    Require(wait(device, 1, 1000, nullptr, 0, &token) == S_OK && token == 0x123456, "Frame point wait changed an opaque token");
    Require(wait(device, 2, 1000, nullptr, 0, &token) == S_OK && token == 0x123456, "Second frame point wait changed its token");
    Reject<std::invalid_argument>([&] { schedule(device, 3, 0, nullptr, 0); });
    Reject<std::invalid_argument>([&] { schedule(device, 1, 0, nullptr, 2); });
    Reject<std::invalid_argument>([&] { schedule(device, 0, 0, nullptr, 1); });
    Reject<std::invalid_argument>([&] { wait(device, 3, 0, nullptr, 0, &token); });
    Reject<std::invalid_argument>([&] { wait(device, 0, 0, nullptr, 0, nullptr); });
    Reject<std::runtime_error>([&] { wait(device, 0, 0, nullptr, 2, &token); });
    Require(schedule(device, 2, 0, nullptr, 1) == S_OK, "Frame point cancellation failed");
    Require(schedule(device, 2, 0, nullptr, 1) == S_FALSE, "Cancelled frame point reported an active schedule");
    Reject<std::invalid_argument>([&] { wait(device, 2, 0, nullptr, 0, &token); });
}
