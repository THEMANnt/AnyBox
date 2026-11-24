#include "Clock.hpp"
#include <anybox/Unimplemented.hpp>

HRESULT AnyBox::Graphics::FrameClock::Wait(std::uint32_t type, std::uint32_t timeout, const FrameAncillary* ancillary, std::uint32_t flags, std::uint64_t* token) {
    if (type >= kinds.size() || !token || (flags & ~3u) != 0) throw std::invalid_argument("Xbox frame wait type, token or flags are invalid");
    if ((flags & 2) != 0) ANYBOX_NOT_IMPLEMENTED();
    auto count = ancillary ? ancillary->count : 0;
    if (ancillary && (count > 31 || (count != 0 && !ancillary->handles))) throw std::invalid_argument("Xbox frame wait ancillary array is invalid");
    if ((flags & 1) != 0 && count != 0) ANYBOX_NOT_IMPLEMENTED();
    std::unique_lock waitLock(waitMutexes[type]);
    std::array<HANDLE, 33> handles{};
    handles[0] = failed.get();
    handles[1] = kinds[type].event.get();
    for (std::uint32_t index = 0; index < count; ++index) {
        handles[index + 2] = ancillary->handles[index];
        if (!handles[index + 2] || handles[index + 2] == INVALID_HANDLE_VALUE) throw std::invalid_argument("Xbox frame wait ancillary handle is invalid");
    }
    {
        std::scoped_lock lock(mutex);
        checkFailure();
        if (!kinds[type].scheduled) throw std::invalid_argument("Xbox frame wait requires a scheduled event");
        if (type == 0 && *token != 0) throw std::invalid_argument("Xbox frame origin token has already been consumed or is foreign");
    }

    auto result = WaitForMultipleObjects(count + 2, handles.data(), FALSE, timeout);
    if (result == WAIT_FAILED) Windows::ThrowLastError("WaitForMultipleObjects Xbox frame event");
    std::scoped_lock lock(mutex);
    checkFailure();
    if (result == WAIT_OBJECT_0) throw std::logic_error("Xbox frame failure event was signaled without an exception");
    std::int32_t signaled;
    if (result == WAIT_TIMEOUT) signaled = -1;
    else if (result == WAIT_OBJECT_0 + 1) {
        signaled = -2;
        if (type == 0) *token = consumeOrigin();
    } else if (result >= WAIT_OBJECT_0 + 2 && result < WAIT_OBJECT_0 + count + 2) signaled = static_cast<std::int32_t>(result - WAIT_OBJECT_0 - 2);
    else throw std::runtime_error("Xbox frame wait returned an unsupported result");
    if (ancillary && ancillary->signaled) *ancillary->signaled = signaled;
    return result == WAIT_TIMEOUT ? S_FALSE : S_OK;
}

std::uint64_t AnyBox::Graphics::FrameClock::consumeOrigin() {
    if (originCount == 0) throw std::logic_error("Xbox frame origin signal has no timestamp");
    if ((nextToken & 0xFFFFFFFFFFFFull) == 0xFFFFFFFFFFFFull) throw std::overflow_error("Xbox frame token sequence is exhausted");
    auto value = ++nextToken;
    records[nextRecord] = {value, origins[originBegin].qpc, counter()};
    nextRecord = (nextRecord + 1) % records.size();
    originBegin = (originBegin + 1) % origins.size();
    --originCount;
    return value;
}
