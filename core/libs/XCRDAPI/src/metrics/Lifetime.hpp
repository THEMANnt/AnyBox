#ifndef ANYBOX_STORAGE_METRICS_LIFETIME_HPP
#define ANYBOX_STORAGE_METRICS_LIFETIME_HPP

#include "Counters.hpp"

namespace AnyBox::Storage {

class QueueStatistics {
public:
    QueueStatistics() = default;
    QueueStatistics(const QueueStatistics&) = delete;
    QueueStatistics& operator=(const QueueStatistics&) = delete;
    ~QueueStatistics();
    void Open(std::int8_t priority) noexcept;
    void Close() noexcept;

private:
    PriorityCounters* counters = nullptr;
};

class FileStatistics {
public:
    FileStatistics() = default;
    FileStatistics(const FileStatistics&) = delete;
    FileStatistics& operator=(const FileStatistics&) = delete;
    ~FileStatistics();
    void Open() noexcept;
    void Close() noexcept;

private:
    bool opened = false;
};

}

#endif
