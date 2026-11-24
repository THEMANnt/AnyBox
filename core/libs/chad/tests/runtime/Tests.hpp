#ifndef ANYBOX_DSP_RUNTIME_TESTS_HPP
#define ANYBOX_DSP_RUNTIME_TESTS_HPP

#include "../dsp/Tests.hpp"

#include <chad.hpp>

#include <atomic>
#include <vector>

namespace AnyBox::Tests {

template<class TException, class TCallable>
void ExpectException(TCallable callable) {
    try {
        callable();
    } catch (const TException&) {
        return;
    }
    throw std::runtime_error("Expected DSP error was not raised");
}

inline XDspConnection* Connect(std::vector<float>& memory, std::uint32_t seconds, std::uint32_t streams) {
    XDspConnection* connection = nullptr;
    Require(XDspConnectWithMaximumStreamLimit(memory.data(), static_cast<std::uint32_t>(memory.size() * sizeof(float)), seconds, streams, 1, &connection) == 0 && connection, "DSP client connection failed");
    return connection;
}

inline void CheckStatus(XDspStatus* status, std::uint32_t count) {
    Require(std::atomic_ref(status->completed).load(std::memory_order_acquire) == count, "DSP completion status is stale");
    Require(status->submitted == count && status->failed == 0 && status->error == 0, "DSP shared status is inconsistent");
}

void CheckFftStreams();
void CheckConvolutionStream();
void CheckClientLimits();
void CheckConcurrentStreams();

}

#endif
