#pragma once

#include "Context.hpp"

namespace AnyBox::Vulkan {
class Timeline {
public:
    explicit Timeline(Context& context, std::uint64_t initialValue = 0);
    static Timeline ImportD3D12Fence(Context& context, HANDLE handle);
    ~Timeline();
    Timeline(const Timeline&) = delete;
    Timeline& operator=(const Timeline&) = delete;
    Timeline(Timeline&& other) noexcept;
    Timeline& operator=(Timeline&& other) = delete;
    VkSemaphore GetHandle() const noexcept { return semaphore; }
    std::uint64_t GetValue() const;
    bool Wait(std::uint64_t value, std::uint64_t timeoutNanoseconds) const;
    void SignalOnQueue(std::uint32_t queueIndex, std::uint64_t value);
    void WaitOnQueue(std::uint32_t queueIndex, std::uint64_t value);

private:
    Context& context;
    VkSemaphore semaphore = VK_NULL_HANDLE;
};
}
