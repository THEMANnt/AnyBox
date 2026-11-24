#include "Tests.hpp"
#include "../../src/frame/Timeline.hpp"

void AnyBox::Graphics::Tests::CheckFrameTimeline() {
    using namespace std::chrono_literals;
    auto base = FrameTime{} + 1s;
    Require(NextFrameDeadline(base, 16667us, 1000us, base) == base + 1000us, "Frame offset was not preserved");
    Require(NextFrameDeadline(base, 16667us, 0us, base) == base + 16667us, "Frame origin deadline was not advanced");
    Require(NextFrameDeadline(base, 16667us, 1000us, base + 1000us) == base + 17667us, "Signaled frame deadline was issued twice");
    Require(NextFrameDeadline(base, 16667us, 1000us, base + 100000us) == base + 101002us, "Missed frame deadlines accumulated drift");
    Reject<std::invalid_argument>([&] { NextFrameDeadline(base, 0us, 0us, base); });
    Reject<std::invalid_argument>([&] { NextFrameDeadline(base, 10us, -1us, base); });
    Reject<std::invalid_argument>([&] { NextFrameDeadline(base, 10us, 10us, base); });
}
