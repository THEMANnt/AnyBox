#include "Lifetime.hpp"

#include <exception>
#include <utility>

AnyBox::Storage::QueueStatistics::~QueueStatistics() { Close(); }

void AnyBox::Storage::QueueStatistics::Open(std::int8_t priority) noexcept {
    if (counters) std::terminate();
    counters = &GetCounters().priorities[static_cast<std::size_t>(priority + 1)];
    auto current = counters->currentQueues.fetch_add(1, std::memory_order_relaxed) + 1;
    UpdateMaximum(counters->maximumQueues, current);
    counters->cumulativeQueues.fetch_add(1, std::memory_order_relaxed);
}

void AnyBox::Storage::QueueStatistics::Close() noexcept {
    auto* current = std::exchange(counters, nullptr);
    if (current && current->currentQueues.fetch_sub(1, std::memory_order_relaxed) == 0) std::terminate();
}

AnyBox::Storage::FileStatistics::~FileStatistics() { Close(); }

void AnyBox::Storage::FileStatistics::Open() noexcept {
    if (opened) std::terminate();
    auto& counters = GetCounters();
    auto current = counters.currentFiles.fetch_add(1, std::memory_order_relaxed) + 1;
    UpdateMaximum(counters.maximumFiles, current);
    counters.cumulativeFiles.fetch_add(1, std::memory_order_relaxed);
    opened = true;
}

void AnyBox::Storage::FileStatistics::Close() noexcept {
    if (!std::exchange(opened, false)) return;
    if (GetCounters().currentFiles.fetch_sub(1, std::memory_order_relaxed) == 0) std::terminate();
}
