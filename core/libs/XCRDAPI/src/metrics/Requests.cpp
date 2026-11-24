#include "Requests.hpp"
#include "../debug/Configuration.hpp"

#include <exception>

AnyBox::Storage::RequestStatistics::RequestStatistics(std::int8_t priority, std::uint32_t sourceBytes, std::uint32_t destinationBytes) noexcept : counters(GetCounters().priorities[static_cast<std::size_t>(priority + 1)]) {
    counters.pendingRequests.fetch_add(1, std::memory_order_relaxed);
    counters.totalRequests.fetch_add(1, std::memory_order_relaxed);
    counters.sourceBytes.fetch_add(sourceBytes, std::memory_order_relaxed);
    counters.destinationBytes.fetch_add(destinationBytes, std::memory_order_relaxed);
}

AnyBox::Storage::RequestStatistics::~RequestStatistics() {
    if (counters.pendingRequests.fetch_sub(1, std::memory_order_relaxed) == 0) std::terminate();
}

AnyBox::Storage::WorkStatistics::WorkStatistics(WorkType type) noexcept : pending(type == WorkType::Storage ? GetCounters().pendingStorage : GetCounters().pendingDecompression), errors(type == WorkType::Storage ? GetCounters().storageErrors : GetCounters().decompressionErrors), exceptions(std::uncaught_exceptions()) {
    pending.fetch_add(1, std::memory_order_relaxed);
    auto& total = type == WorkType::Storage ? GetCounters().totalStorage : GetCounters().totalDecompression;
    total.fetch_add(1, std::memory_order_relaxed);
}

AnyBox::Storage::WorkStatistics::~WorkStatistics() {
    if (reportErrors && std::uncaught_exceptions() > exceptions) errors.fetch_add(1, std::memory_order_relaxed);
    if (pending.fetch_sub(1, std::memory_order_relaxed) == 0) std::terminate();
}

void AnyBox::Storage::WorkStatistics::Cancel() noexcept { reportErrors = false; }

AnyBox::Storage::RequestValidation::RequestValidation(std::source_location location) noexcept : exceptions(std::uncaught_exceptions()), function(location.function_name()) {}

AnyBox::Storage::RequestValidation::~RequestValidation() {
    if (std::uncaught_exceptions() <= exceptions) return;
    GetCounters().errors.fetch_add(1, std::memory_order_relaxed);
    ReportValidationFailure(function);
}
