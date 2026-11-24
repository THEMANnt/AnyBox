#include "Tests.hpp"
#include "../queue/Tests.hpp"

#include <array>

namespace {

struct UnwindProbe {
    unsigned& count;
    ~UnwindProbe() { ++count; }
};

}

void AnyBox::Tests::CheckDebugChild(unsigned flags) {
    QueueFixture fixture(1);
    ExpectException<std::invalid_argument>([&] { fixture.factory->SetDebugFlags(8); });
    ExpectException<std::runtime_error>([&] { fixture.factory->SetDebugFlags(4); });
    fixture.factory->SetDebugFlags(flags);
    unsigned unwound = 0;
    ExpectException<std::invalid_argument>([&] { UnwindProbe probe{unwound}; fixture.queue->EnqueueRequest(nullptr); });
    ExpectException<std::invalid_argument>([&] { UnwindProbe probe{unwound}; fixture.queue->EnqueueStatus(nullptr, 0); });
    ExpectException<std::invalid_argument>([&] { UnwindProbe probe{unwound}; fixture.queue->EnqueueSignal(nullptr, 0); });
    ExpectException<std::system_error>([&] { UnwindProbe probe{unwound}; fixture.queue->EnqueueSetEvent(nullptr); });
    Require(unwound == 4, "DirectStorage debug breaks interrupted C++ stack unwinding");

    std::array<std::byte, 8> source{};
    std::array<std::byte, 32> destination{};
    StorageRequest request{};
    request.options = 1 | (1ULL << 22);
    request.source = source.data();
    request.sourceSize = static_cast<std::uint32_t>(source.size());
    request.destination = destination.data();
    request.destinationSize = static_cast<std::uint32_t>(destination.size());
    fixture.queue->EnqueueRequest(&request);
    fixture.Wait();
    Require(fixture.status->GetHResult(0) < 0, "Asynchronous DirectStorage error was lost in debug mode");
    fixture.factory->SetDebugFlags(0);
}
