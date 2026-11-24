#ifndef ANYBOX_STORAGE_REQUEST_METRICS_HPP
#define ANYBOX_STORAGE_REQUEST_METRICS_HPP

#include "Counters.hpp"

#include <source_location>

namespace AnyBox::Storage {

class RequestStatistics {
public:
    RequestStatistics(std::int8_t priority, std::uint32_t sourceBytes, std::uint32_t destinationBytes) noexcept;
    RequestStatistics(const RequestStatistics&) = delete;
    RequestStatistics& operator=(const RequestStatistics&) = delete;
    ~RequestStatistics();

private:
    PriorityCounters& counters;
};

enum class WorkType { Storage, Decompression };

class WorkStatistics {
public:
    explicit WorkStatistics(WorkType type) noexcept;
    WorkStatistics(const WorkStatistics&) = delete;
    WorkStatistics& operator=(const WorkStatistics&) = delete;
    ~WorkStatistics();
    void Cancel() noexcept;

private:
    std::atomic<std::uint32_t>& pending;
    std::atomic<std::uint32_t>& errors;
    int exceptions;
    bool reportErrors = true;
};

class RequestValidation {
public:
    explicit RequestValidation(std::source_location location = std::source_location::current()) noexcept;
    RequestValidation(const RequestValidation&) = delete;
    RequestValidation& operator=(const RequestValidation&) = delete;
    ~RequestValidation();

private:
    int exceptions;
    const char* function;
};

}

#endif
