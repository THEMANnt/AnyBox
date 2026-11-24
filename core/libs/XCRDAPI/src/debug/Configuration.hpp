#ifndef ANYBOX_STORAGE_DEBUG_CONFIGURATION_HPP
#define ANYBOX_STORAGE_DEBUG_CONFIGURATION_HPP

#include <atomic>
#include <cstdint>

namespace AnyBox::Storage {

std::atomic<std::uint32_t>& GetDebugFlags() noexcept;
void ReportValidationFailure(const char* function) noexcept;

}

#endif
