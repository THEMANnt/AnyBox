#pragma once

#include "Ancillary.hpp"
#include "Configuration.hpp"
#include "Timeline.hpp"
#include <chrono>
#include <exception>
#include <mutex>
#include <thread>

namespace AnyBox::Graphics {
class FrameClock {
public:
    FrameClock();
    ~FrameClock();
    FrameClock(const FrameClock&) = delete;
    FrameClock& operator=(const FrameClock&) = delete;
    HRESULT Schedule(FrameConfiguration configuration, std::uint32_t type, std::uint32_t offset, const FrameAncillary* ancillary, std::uint32_t flags);
    HRESULT Wait(std::uint32_t type, std::uint32_t timeout, const FrameAncillary* ancillary, std::uint32_t flags, std::uint64_t* token);

private:
    struct Kind {
        FrameHandle event;
        FrameSignals signals{nullptr};
        FrameTime deadline{};
        std::uint32_t offset = 0;
        bool scheduled = false;
    };
    struct Origin {
        std::int64_t qpc;
    };
    struct Record {
        std::uint64_t token = 0;
        std::int64_t originQpc = 0;
        std::int64_t waitQpc = 0;
    };
    void run(std::stop_token stop) noexcept;
    void tick();
    void publish(std::size_t type);
    void checkFailure() const;
    std::uint64_t consumeOrigin();
    static std::int64_t counter();
    FrameHandle wake;
    FrameHandle failed;
    FrameHandle timer;
    std::array<Kind, 3> kinds;
    std::array<std::mutex, 3> waitMutexes;
    std::mutex mutex;
    FrameConfiguration configuration;
    FrameTime base{};
    std::exception_ptr failure;
    std::array<Origin, 5> origins{};
    std::size_t originBegin = 0;
    std::size_t originCount = 0;
    std::array<Record, 18> records{};
    std::uint64_t nextToken;
    std::size_t nextRecord = 0;
    std::jthread worker;
};
}
